#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

#include "optimizations/analysis/loop_analysis.h"

namespace compiler {

void Inst::SetControlInput(Inst *inst) {
    ASSERT(inst != nullptr);
    ASSERT(HasControlProp());
    SetRawInput(0, inst);
    inst->SetControlUser(this);
}

Inst *Inst::GetControlInput() {
    ASSERT(HasControlProp());
    return GetRawInput(0);
}

void Inst::SetControlUser(Inst *inst) {
    ASSERT(HasControlProp());
    if (GetRawUsers().size() == 0) {
        GetRawUsers().push_back(inst);
        return;
    }
    *GetRawUsers().begin() = inst;
}

Inst *Inst::GetControlUser() {
    ASSERT(HasControlProp());
    ASSERT(GetRawUsers().size() > 0);
    return *GetRawUsers().begin();
}

void Inst::SetDataInput(id_t index, Inst *inst) {
    ASSERT(inst != nullptr);
    ASSERT(index <= NumDataInputs());
    if (HasControlProp()) {
        index++;
    }
    // For dynamic inputs
    if (index != NumAllInputs() && GetRawInput(index) != nullptr) {
        GetRawInput(index)->DeleteDataUser(this);
    }
    SetRawInput(index, inst);
    inst->AddDataUser(this);
}

Inst *Inst::GetDataInput(id_t index) {
    ASSERT(!IsRegion());  // RegionInst has special method GetRegionInput
    ASSERT(index < NumDataInputs());
    if (HasControlProp()) {
        index++;
    }
    return GetRawInput(index);
}

const std::list<Inst *> Inst::GetDataUsers() {
    return std::list<Inst *>(StartIteratorDataUsers(), GetRawUsers().end());
}

void Inst::AddDataUser(Inst *inst) {
    if (HasControlProp() && GetRawUsers().size() == 0) {
        GetRawUsers().push_back(nullptr);
    }
    auto it = std::find(StartIteratorDataUsers(), GetRawUsers().end(), inst);
    if (it != GetRawUsers().end()) {
        return;
    }
    GetRawUsers().push_back(inst);
}

void Inst::DeleteDataUser(Inst *inst) {
    auto it = std::find(StartIteratorDataUsers(), GetRawUsers().end(), inst);
    ASSERT(it != GetRawUsers().end());
    users_.erase(it);
}

void Inst::DeleteRawUser(Inst *inst) {
    auto it = std::find(GetRawUsers().begin(), GetRawUsers().end(), inst);
    if (it != GetRawUsers().end()) {
        users_.erase(it);
    }
}

uint32_t Inst::NumDataUsers() {
    return HasControlProp() ? GetRawUsers().size() - 1 : GetRawUsers().size();
}

void DynamicInputs::SetRawInput(id_t index, Inst *inst) {
    ASSERT(index <= NumAllInputs());
    if (index == NumAllInputs()) {
        inputs_.push_back(inst);
        return;
    }
    auto it = inputs_.begin();
    for (id_t i = 0; i < index; i++) {
        it++;
    }
    *it = inst;
}

Inst *DynamicInputs::GetRawInput(id_t index) {
    auto it = inputs_.begin();
    for (id_t i = 0; i < index; i++) {
        it++;
    }
    return *it;
}

void Inst::Dump(std::ostream& out) {
    out << std::setw(4) << std::right << std::to_string(GetId()) << std::string(".") << std::setw(4) << std::left << TypeToString(GetType());
    DumpOpcode(out);
    DumpInputs(out);
    DumpUsers(out);
    out << std::endl;
}

void Inst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ");
}

void CompareInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ") << CcToString(GetCC()) << std::string(" ");
}

void RegionInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ");
    if (loop_ == nullptr) {
        return;
    }

    out << std::string("[Loop:");
    if (GetLoop()->GetId() == 0) {
        out << "root";
    } else {
        out << GetLoop()->GetId() << ", Depth:" << GetLoop()->GetDepth();
    }
    if (loop_->GetHeader() == this) {
        out << std::string(", Header");
    }
    auto backedges = loop_->GetBackedges();
    if (std::find(backedges.begin(), backedges.end(), this) != backedges.end()) {
        out << std::string(", Backedge");
    }
    if (loop_->IsIrreducible()) {
        out << std::string(", Irreducible");
    }
    out << std::string("] ");
}

void Inst::DumpUsers(std::ostream& out) {
    if (GetRawUsers().size() == 0) {
        return;
    }
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetRawUsers()) {
        out << std::string(first ? "" : ", ");
        if (inst == nullptr) {
            out << std::string("NOT_SET");
            first = false;
            continue;
        }
        out << std::string("v") << std::to_string(inst->GetId());
        first = false;
    }
}

void IfInst::DumpUsers(std::ostream &out) {
    ASSERT(GetRawUsers().size() == 2);
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetRawUsers()) {
        out << std::string(first ? "" : ", ");
        if (inst == nullptr) {
            out << std::string("NOT_SET");
            first = false;
            continue;
        }
        out << std::string(first ? "T:" : "F:") << std::string("v") << std::to_string(inst->GetId());
        first = false;
    }
}

void PhiInst::DumpInputs(std::ostream &out) {
    auto region = static_cast<RegionInst *>(GetControlInput());
    if (NumDataInputs() != region->NumAllInputs()) {
        std::cerr << "Num inputs in PHI and regions not equal!\n";
        std::exit(1);
    }
    uint32_t i = 0;
    out << std::string("v") << std::to_string(GetControlInput()->GetId());
    for (auto inst : GetDataInputs()) {
        out << std::string(", ") << std::string("v") << std::to_string(inst->GetId()) << std::string("(R")
            << std::to_string((region->GetRegionInput(i))->GetId()) << std::string(")");
        i++;
    }
}

void DynamicInputs::DumpInputs(std::ostream &out) {
    bool first = true;
    for (auto inst : GetAllInputs()) {
        if (inst == nullptr) {
            continue;
        }
        out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
        first = false;
    }
}

std::string OpcodeToString(Opcode opc) {
    return OPCODE_NAME.at(static_cast<size_t>(opc));
}

std::string TypeToString(Type type) {
    return TYPE_NAME.at(static_cast<size_t>(type));
}

std::string CcToString(ConditionCode cc) {
    return CC_NAME.at(static_cast<size_t>(cc));
}

RegionInst::~RegionInst() {
    if (loop_ != nullptr) {
        delete loop_;
    }
}

bool RegionInst::IsLoopHeader() {
    return GetLoop()->GetHeader() == this;
}

void RegionInst::AddFirstInst(Inst *inst) {
    ASSERT(first_ == nullptr && last_ == nullptr);
    first_ = inst;
    last_ = inst;
}

void RegionInst::PushBackInst(Inst *inst) {
    inst->SetPlaced();
    if (first_ == nullptr) {
        AddFirstInst(inst);
        return;
    }

    last_->SetNext(inst);
    inst->SetPrev(last_);
    last_ = inst;
}

void RegionInst::PushFrontInst(Inst *inst) {
    inst->SetPlaced();
    if (first_ == nullptr) {
        AddFirstInst(inst);
        return;
    }

    first_->SetPrev(inst);
    inst->SetNext(first_);
    first_ = inst;
}

RegionInst *Inst::CastToRegion() {
    ASSERT(IsRegion());
    return static_cast<RegionInst *>(this);
}

IfInst *Inst::CastToIf() {
    ASSERT(GetOpcode() == Opcode::If);
    return static_cast<IfInst *>(this);
}

ConstantInst *Inst::CastToConstant() {
    ASSERT(GetOpcode() == Opcode::Constant);
    return static_cast<ConstantInst *>(this);
}

ParameterInst *Inst::CastToParameter() {
    ASSERT(GetOpcode() == Opcode::Parameter);
    return static_cast<ParameterInst *>(this);
}

CallInst *Inst::CastToCall() {
    ASSERT(GetOpcode() == Opcode::Call);
    return static_cast<CallInst *>(this);
}

JumpInst *Inst::CastToJump() {
    ASSERT(GetOpcode() == Opcode::Jump);
    return static_cast<JumpInst *>(this);
}

void Inst::ReplaceDataUsers(Inst *from) {
    for (auto it = from->StartIteratorDataUsers(); it != from->GetRawUsers().end(); it = from->StartIteratorDataUsers()) {
        auto user = *it;
        if (user == nullptr) {
            continue;
        }
        for (id_t i = 0; i < user->NumDataInputs(); i++) {
            if (user->GetDataInput(i) == from) {
                user->SetDataInput(i, this);
            }
        }
    }
    from->GetRawUsers().erase(from->StartIteratorDataUsers(), from->GetRawUsers().end());
}

void CallInst::SetNameFunc(const std::string &name) {
    name_func_ = name;
}

std::string CallInst::GetNameFunc() const {
    return name_func_;
}

Inst *Inst::LiteClone(Graph *target_graph, std::map<id_t, id_t> &connect) {
    auto new_inst = target_graph->CreateClearInstByOpcode(GetOpcode());
    new_inst->type_ = type_;
    new_inst->SetId(target_graph->GetNumInsts());
    target_graph->AddInst(new_inst);

    // To many specific cases in common code!
    auto opc = GetOpcode();
    if (opc == Opcode::Start || opc == Opcode::Parameter || opc == Opcode::Region || opc == Opcode::End) {
        return new_inst;
    }
    // if (opc == Opcode::Region || opc == Opcode::End) {
    //     auto num_inputs = CastToRegion()->NumRegionInputs();
    //     for (id_t index = 0; index < num_inputs; index++) {
    //         new_inst->CastToRegion()->SetRegionInput(index, target_graph->GetInstByIndex(connect[CastToRegion()->GetRegionInput(index)->GetId()]));
    //     }
    //     return new_inst;
    // }

    if (HasControlProp() && GetControlInput() != nullptr) {
        new_inst->SetControlInput(target_graph->GetInstByIndex(connect[GetControlInput()->GetId()]));
    }
    auto num_inputs = NumDataInputs();
    for (id_t index = 0; index < num_inputs; index++) {
        // If id_t > num_all_inst in graph, write input in order (case for Jump and If)
        new_inst->SetDataInput(index, target_graph->GetInstByIndex(connect[GetDataInput(index)->GetId()]));
    }
    return new_inst;
}

Inst *ConstantInst::LiteClone(Graph *target_graph, std::map<id_t, id_t> &connect) {
    auto new_inst = static_cast<ConstantInst *>(Inst::LiteClone(target_graph, connect));
    new_inst->SetImm(GetImm());
    return new_inst;
}

Inst *CompareInst::LiteClone(Graph *target_graph, std::map<id_t, id_t> &connect) {
    auto new_inst = static_cast<CompareInst *>(FixedInputs<2>::LiteClone(target_graph, connect));
    new_inst->SetCC(GetCC());
    return new_inst;
}

Inst *ParameterInst::LiteClone(Graph *target_graph, std::map<id_t, id_t> &connect) {
    auto new_inst = static_cast<ParameterInst *>(Inst::LiteClone(target_graph, connect));
    new_inst->SetIndexParam(GetIndexParam());
    return new_inst;
}

Inst *CallInst::LiteClone(Graph *target_graph, std::map<id_t, id_t> &connect) {
    auto new_inst = static_cast<CallInst *>(Inst::LiteClone(target_graph, connect));
    new_inst->SetNameFunc(GetNameFunc());
    return new_inst;
}

void CallInst::DumpInputs(std::ostream &out) {
    out << "\"" << GetNameFunc() << "\" ";
    Base::DumpInputs(out);
}

void ParameterInst::DumpInputs(std::ostream &out) {
    out << "\"" << GetIndexParam() << "\" ";
    Inst::DumpInputs(out);
}

void Inst::ReplaceAllUsers(Inst *from) {
    ReplaceDataUsers(from);
    ReplaceCtrUser(from);
}

void Inst::ReplaceCtrUser(Inst *from) {
    ASSERT(HasSingleDataUser());
    auto c_user = from->GetControlUser();
    if (c_user == nullptr) {
        return;
    }
    SetControlUser(c_user);
    from->SetControlUser(nullptr);
}

void DynamicInputs::DeleteInput(Inst *inst) {
    auto it = std::find(inputs_.begin(), inputs_.end(), inst);
    ASSERT(it != inputs_.end());
    inputs_.erase(it);
}

}
