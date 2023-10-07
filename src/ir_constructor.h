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
        assert(current_inst_ != nullptr);

        switch(current_inst_->GetOpcode()) {
            case Opcode::Constant: {
                static_cast<ConstantInst *>(current_inst_)->SetImm(imm);
                break;
            }
            default: {
                assert(false && ("Should be unreachable!"));
            }
        }
        return *this;
    }

    IrConstructor &CC(ConditionCode cc) {
        assert(current_inst_ != nullptr);

        switch(current_inst_->GetOpcode()) {
            case Opcode::Compare: {
                static_cast<CompareInst *>(current_inst_)->SetCC(cc);
                break;
            }
            default: {
                assert(false && ("Should be unreachable!"));
            }
        }
        return *this;
    }

    IrConstructor &Branches(id_t true_br, id_t false_br) {
        assert(current_inst_ != nullptr);
        if (current_inst_->GetOpcode() != Opcode::If) {
            assert(false && ("Should be unreachable!"));
        }
        static_cast<IfInst *>(current_inst_)->SetTrueBranch(GetGraph()->GetInstByIndex(true_br));
        static_cast<IfInst *>(current_inst_)->SetFalseBranch(GetGraph()->GetInstByIndex(false_br));
        return *this;
    }

    template<typename... Args>
    IrConstructor &Inputs(Args ...args) {
        for (auto it : {args...}) {
            static_assert(std::is_same<decltype(it), int>(), "Is not \"Int\" in argument");
        }
        auto inputs = std::vector<int>({args...});
        for (int i = 0; i < inputs.size(); i++) {
            current_inst_->SetInput(i, graph_->GetInstByIndex(inputs.at(i)));
        }
        return *this;
    }

    Graph *GetGraph() {
        return graph_;
    }

private:
    Graph *graph_;
    Inst *current_inst_;
};

}
