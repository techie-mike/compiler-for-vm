#include "graph_comparator.h"
#include "inst.h"

namespace compiler {

void GraphComparator::Compare() {
    ASSERT_EQ(left_->GetMethodName(), right_->GetMethodName());
    ASSERT_EQ(left_->GetNumInsts(), right_->GetNumInsts());
    for (size_t i = 0; i < left_->GetNumInsts(); i++) {
        auto left_inst = left_->GetInstByIndex(i);
        auto right_inst = right_->GetInstByIndex(i);

        CompareInstructions(left_inst, right_inst);
    }
}

void GraphComparator::CompareInstructions(Inst *left, Inst *right) {
    ASSERT_EQ(left->GetId(), right->GetId());
    ASSERT_EQ(left->GetOpcode(), right->GetOpcode());
    ASSERT_EQ(left->GetType(), right->GetType());
    CompareInputs(left, right);

    // Specific checks
    switch (left->GetOpcode()) {
        case Opcode::Constant: {
            ASSERT_EQ(static_cast<ConstantInst *>(left)->GetImm(), static_cast<ConstantInst *>(right)->GetImm());
            break;
        }
        case Opcode::Compare: {
            ASSERT_EQ(static_cast<CompareInst *>(left)->GetCC(), static_cast<CompareInst *>(right)->GetCC());
            break;
        }
        default: {
            break;
        }
    }
}

void GraphComparator::CompareInputs(Inst *left, Inst *right) {
    ASSERT_EQ(left->NumAllInputs(), right->NumAllInputs());
    if (left->NumAllInputs() == 0) {
        return;
    }

    for (size_t i = 0; i < left->NumAllInputs(); i++) {
        ASSERT_EQ(left->GetRawInput(i)->GetId(), right->GetRawInput(i)->GetId());
    }
}

}
