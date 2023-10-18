#pragma once

#include "graph.h"
#include "inst.h"
#include <type_traits>

namespace compiler {

class IrConstructor {
public:
    IrConstructor():
        graph_(new Graph()) {
            graph_->SetUnitTestMode();
        };

    ~IrConstructor() {
        delete graph_;
    }
    template<Opcode T>
    IrConstructor &CreateInst(id_t index) {
        auto inst = graph_->CreateInstByIndex<T>(index);
        current_inst_ = inst;
        return *this;
    }

    IrConstructor &Imm(int64_t imm) {
        ASSERT(current_inst_ != nullptr);

        switch(current_inst_->GetOpcode()) {
            case Opcode::Constant: {
                static_cast<ConstantInst *>(current_inst_)->SetImm(imm);
                break;
            }
            case Opcode::Parameter: {
                static_cast<ParameterInst *>(current_inst_)->SetIndexParam(imm);
                break;
            }
            default: {
                ASSERT(false && ("Should be unreachable!"));
            }
        }
        return *this;
    }

    IrConstructor &CC(ConditionCode cc) {
        ASSERT(current_inst_ != nullptr);

        switch(current_inst_->GetOpcode()) {
            case Opcode::Compare: {
                static_cast<CompareInst *>(current_inst_)->SetCC(cc);
                break;
            }
            default: {
                ASSERT(false && ("Should be unreachable!"));
            }
        }
        return *this;
    }

    IrConstructor &Branches(id_t true_br, id_t false_br) {
        ASSERT(current_inst_ != nullptr);
        if (current_inst_->GetOpcode() != Opcode::If) {
            UNREACHABLE();
        }
        ifs_.push_back({current_inst_->GetId(), true_br, false_br});
        return *this;
    }

    IrConstructor &JmpTo(id_t id) {
        if (current_inst_->GetOpcode() != Opcode::Jump) {
            UNREACHABLE();
        }
        jmps_.push_back({current_inst_->GetId(), id});
        return *this;
    }

    template<typename... Args>
    IrConstructor &DataInputs(Args ...args) {
        for (auto it : {args...}) {
            static_assert(std::is_same<decltype(it), int>(), "Is not \"Int\" in argument");
        }
        auto inputs = std::vector<int>({args...});
        for (size_t i = 0; i < inputs.size(); i++) {
            current_inst_->SetDataInput(i, graph_->GetInstByIndex(inputs.at(i)));
        }
        return *this;
    }

    IrConstructor &CtrlInput(id_t id) {
        auto inst = GetGraph()->GetInstByIndex(id);
        current_inst_->SetControlInput(inst);
        return *this;
    }

    void FinalizeRegions() {
        if (finalized_) {
            return;
        }
        for (auto &jmp : jmps_) {
            auto inst = static_cast<JumpInst *>(GetGraph()->GetInstByIndex(jmp.id));
            auto region = GetGraph()->GetInstByIndex(jmp.jmp_to);
            inst->SetJmpTo(region);
        }

        for (auto &if_it : ifs_) {
            auto inst = static_cast<IfInst *>(GetGraph()->GetInstByIndex(if_it.id));
            auto region_true = GetGraph()->GetInstByIndex(if_it.jmp_true);
            auto region_false = GetGraph()->GetInstByIndex(if_it.jmp_false);

            inst->SetTrueBranch(region_true);
            inst->SetFalseBranch(region_false);
        }
        finalized_ = true;
    }
    Graph *GetFinalGraph() {
        if (!finalized_) {
            FinalizeRegions();
        }
        return graph_;
    }

private:
    Graph *GetGraph() {
        return graph_;
    }
    struct Jump {
        id_t id;
        id_t jmp_to;
    };

    struct If {
        id_t id;
        id_t jmp_true;
        id_t jmp_false;
    };

    std::vector<Jump> jmps_;
    std::vector<If> ifs_;

private:
    Graph *graph_;
    Inst *current_inst_;
    bool finalized_ = false;
};

}
