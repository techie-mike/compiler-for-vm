#include <limits>

#include "peepholes.h"
#include "analysis/rpo.h"
#include "constant_folding.h"

namespace compiler {

void Peepholes::Run() {
    auto rpo = RpoInsts(graph_);
    rpo.Run();
    auto rpo_vector = rpo.GetVector();
    for (auto inst : rpo_vector) {
        AnalysisInst(inst);
    }
}

void Peepholes::AnalysisInst(Inst *inst) {
    switch (inst->GetOpcode()) {
        case Opcode::Sub:
            VisitSub(inst);
            break;
        case Opcode::Shl:
            VisitShl(inst);
            break;
        case Opcode::Or:
            VisitOr(inst);
            break;
        default:
            break;
    }
}

void Peepholes::VisitSub(Inst *inst) {
    if (ConstFoldingBinaryOp(graph_, inst)) {
        return;
    }

    if (TryOptimizeSubZero(inst)) {
        return;
    }
}

void Peepholes::VisitShl(Inst *inst) {
    if (ConstFoldingBinaryOp(graph_, inst)) {
        return;
    }

    if (TryOptimizeShlAfterShr(inst)) {
        return;
    }
}

void Peepholes::VisitOr(Inst *inst) {
    if (ConstFoldingBinaryOp(graph_, inst)) {
        return;
    }
    if (TryOptimizeOrZero(inst)) {
        return;
    }
}

bool Peepholes::TryOptimizeSubZero(Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (input1->IsConst() && input1->CastToConstant()->GetImm() == 0) {
        input0->ReplaceDataUsers(inst);
        return true;
    }
    return false;
}

bool Peepholes::TryOptimizeShlAfterShr(Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (input1->IsConst() &&
        input0->GetOpcode() == Opcode::Shr &&
        input0->GetDataInput(1)->IsConst() &&
        input0->GetDataInput(1)->CastToConstant()->GetImm() == input1->CastToConstant()->GetImm()) {

        auto input_shr = input0->GetDataInput(0);
        auto shift = input1->CastToConstant()->GetImm();
        auto new_const = graph_->CreateConstantInst(std::numeric_limits<ImmType>::max() & (0xffffffffffffffff << shift));
        auto inst_and = graph_->CreateAndInst(input_shr->GetType(), input_shr, new_const);
        inst_and->ReplaceDataUsers(inst);
        return true;
    }
    return false;
}

bool Peepholes::TryOptimizeOrZero(Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (input0->IsConst()) {
        std::swap(input0, input1);
    }

    if (input1->IsConst() && input1->CastToConstant()->GetImm() == 0) {
        input0->ReplaceDataUsers(inst);
        return true;
    }
    return false;
}

}
