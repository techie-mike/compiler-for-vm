#include "inlining.h"
#include "analysis/rpo.h"

namespace compiler {

Inlining::Inlining(Graph *main_graph, std::vector<Graph *> additional_graphs):
    graph_(main_graph) {
    FillMapAdditionalGraphs(additional_graphs);
}

void Inlining::Run() {
    // Find call and try to inline them:
    // 1) Find Call in RPO
    // 2) Find callee method in additioanal graphs
    // Checks:
    // 1) Build graph (in our case already built)
    // 2) Do something optimization
    // 3) Calculate number of instructions
    FindAllCalls();
    TryInlineCalls();
}

void Inlining::TryInlineCalls() {
    for (auto inst : all_calls_) {
        inst->Dump(std::cerr);
        auto call = inst->CastToCall();
        auto func_graph = GetGraphFuncByCall(call);
        if (!func_graph.has_value()) {
            std::cerr << "Not found \n";
            continue;
        }
        if (!CanBeInlined(func_graph.value())) {
            std::cerr << "Bad \n";
            continue;
        }
        InlineFunc(call, func_graph.value());
        already_inlined_insts_ += func_graph.value()->GetNumInsts();
    }
}

void Inlining::InlineFunc([[maybe_unused]]CallInst *call, Graph *func_graph) {
    inlined_subgraph_.clear();
    last_new_cfg_ = nullptr;
    InsertExternalGraph(func_graph);
    UpdateParameters(call);
    UpdateReturn(call);
    UpdateCfgSubgraph(call);
    DeleteUnnecessaryInst();
}

Inst *FindFirstJump(Inst *inst) {
    while (inst->GetOpcode() != Opcode::Jump) {
        inst = inst->GetControlUser();
    }
    return inst;
}

void Inlining::DeleteUnnecessaryInst() {
    graph_->DeleteInst(inlined_subgraph_.back());
    graph_->DeleteInst(inlined_subgraph_.front());
}

void Inlining::UpdateCfgSubgraph(CallInst *call) {
    ASSERT(last_new_cfg_ != nullptr);
    auto lower_connect = call->GetControlUser();
    auto upper_connect = call->GetControlInput();
    graph_->DeleteInst(call);
    FindFirstJump(inlined_subgraph_.front())->SetControlInput(upper_connect);
    lower_connect->SetControlInput(last_new_cfg_);
}

void Inlining::UpdateReturn(Inst *call) {
    auto end_region = inlined_subgraph_.back()->CastToRegion();
    auto num_returns = end_region->NumAllInputs();
    if (num_returns == 1) {
        SingleReturn(call, end_region);
    } else {
        MultipleReturn(call, end_region, num_returns);
    }
}

void Inlining::SingleReturn(Inst *call, RegionInst *end_region) {
    auto last_jump = end_region->GetRegionInput(0);
    ASSERT(last_jump->GetOpcode() == Opcode::Jump);
    auto inlined_return = last_jump->GetControlInput();
    ASSERT(inlined_return->GetOpcode() == Opcode::Return);
    auto return_value = inlined_return->GetDataInput(0);
    graph_->DeleteInst(inlined_return);
    return_value->ReplaceDataUsers(call);
    last_jump->SetControlInput(inlined_return->GetControlInput());
    last_new_cfg_ = graph_->CreateRegionInst();
    last_new_cfg_->SetControlInput(last_jump);
}

void Inlining::MultipleReturn(Inst *call, RegionInst *end_region, uint32_t num_returns) {
    auto sum_region = graph_->CreateRegionInst();
    auto sum_phi = graph_->CreatePhiInst();
    sum_phi->SetControlInput(sum_region);

    for (uint32_t i = 0; i < num_returns; i++) {
        // Find multiple return, on each way to End region
        auto last_jump = end_region->GetRegionInput(i);
        ASSERT(last_jump->GetOpcode() == Opcode::Jump);
        auto inlined_return = last_jump->GetControlInput();
        ASSERT(inlined_return->GetOpcode() == Opcode::Return);
        // Take return value (data input) and delete them
        auto return_value = inlined_return->GetDataInput(0);
        graph_->DeleteInst(inlined_return);
        last_jump->SetControlInput(inlined_return->GetControlInput());

        sum_region->SetRegionInput(i, last_jump);
        last_jump->SetControlUser(sum_region);
        sum_phi->SetDataInput(i, return_value);
    }
    sum_phi->ReplaceDataUsers(call);
    sum_phi->SetControlInput(call->GetControlUser());
    last_new_cfg_ = sum_phi;
}

void Inlining::UpdateParameters(CallInst *call) {
    [[maybe_unused]] auto num_params = call->NumDataInputs();
    for (id_t index = 0; index < num_params; index++) {
        auto comparator = [index](Inst *inst) {
            if (inst->GetOpcode() != Opcode::Parameter) {
                return false;
            }
            return inst->CastToParameter()->GetIndexParam() == index;
        };
        auto it_param = std::find_if(inlined_subgraph_.begin(), inlined_subgraph_.end(), comparator);
        ASSERT(it_param != inlined_subgraph_.end());

        auto argument = call->GetDataInput(index);
        argument->ReplaceDataUsers(*it_param);
        graph_->DeleteInst(*it_param);
    }
}

Inst *Inlining::InsertExternalGraph(Graph *ext_graph) {
    auto rpo = RpoInsts(ext_graph);
    rpo.Run();
    auto rpo_vector = rpo.GetVector();
    std::vector<Inst *> branchs_fill_after;
    std::map<id_t, id_t> connection_index;

    const id_t offset_index = graph_->GetNumInsts();
    // Exclude Start and End insts (first and last in rpo)
    for (auto inst : rpo_vector) {
        auto new_inst = inst->LiteClone(graph_, connection_index);
        inlined_subgraph_.push_back(new_inst);
        connection_index[inst->GetId()] = new_inst->GetId();

        Opcode opc = new_inst->GetOpcode();
        if (opc == Opcode::Jump || opc == Opcode::If) {
            branchs_fill_after.push_back(inst);
        }
    }

    for (auto inst : branchs_fill_after) {
        auto old_index = inst->GetId();
        auto new_inst = graph_->GetInstByIndex(connection_index[inst->GetId()]);
        if (inst->GetOpcode() == Opcode::Jump) {
            auto new_jmp_to = connection_index[ext_graph->GetInstByIndex(old_index)->CastToJump()->GetJumpTo()->GetId()];
            new_inst->CastToJump()->SetJmpTo(graph_->GetInstByIndex(new_jmp_to));
            continue;
        }
        ASSERT(inst->GetOpcode() == Opcode::If);
        auto new_true_branch = connection_index[inst->CastToIf()->GetTrueBranch()->GetId()];
        auto new_false_branch = connection_index[inst->CastToIf()->GetFalseBranch()->GetId()];
        new_inst->CastToIf()->SetTrueBranch(graph_->GetInstByIndex(new_true_branch));
        new_inst->CastToIf()->SetFalseBranch(graph_->GetInstByIndex(new_false_branch));
    }
    return graph_->GetInstByIndex(offset_index);
}

std::optional<Graph *> Inlining::GetGraphFuncByCall(CallInst *call) {
    const auto num_params = call->NumDataInputs();
    const auto &name_func = call->GetNameFunc();
    auto it = map_.find({name_func, num_params});
    if (it == map_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Inlining::CanBeInlined(const Graph *ext_call) {
    // Special attribute in name for disable inlining for function
    if (ext_call->GetMethodName().find("__noinline__") != std::string::npos) {
        return false;
    }
    return ext_call->GetNumInsts() + already_inlined_insts_ <= max_inline_insts_;
}

std::vector<Inst *> Inlining::GetRPOVector() {
    auto rpo = RpoInsts(graph_);
    rpo.Run();
    return rpo.GetVector();
}

void Inlining::FillMapAdditionalGraphs(std::vector<Graph *> &additional_graphs) {
    for (auto graph : additional_graphs) {
        if (map_.find({graph->GetMethodName(), graph->GetNumParams()}) != map_.end()) {
            std::cerr << "Method \"" << graph->GetMethodName() << "\" with " << graph->GetNumParams() << " param(s) is already exist!\n";
            exit(1);
        }
        map_[{graph->GetMethodName(), graph->GetNumParams()}] = graph;
    }
}

void Inlining::FindAllCalls() {
    auto rpo_vector = GetRPOVector();
    for (auto inst : rpo_vector) {
        inst->Dump(std::cerr);
        if (!inst->IsCall()) {
            continue;
        }
        all_calls_.push_back(inst);
    }
}


}
