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
    // Some optimization may be applied, so we don't go out from function
    TryOptimizeSubSub(inst);

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

// 1. Constant 0x0 -> v3
// 2. ... -> v3
// 3. Sub v2, v1 -> v4
// ==========>>==========
// 1. Constant 0x0
// 2. ... -> v4
// 3. Sub v2, v1
bool Peepholes::TryOptimizeSubZero(Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (input1->IsConst() && input1->CastToConstant()->GetImm() == 0) {
        input0->ReplaceDataUsers(inst);
        return true;
    }
    return false;
}

// Optimize sub of sub
// 1. Constant ... -> v3
// 2. Constant ... -> v3
// 3. ... -> v4
// 4. Sub v3, v1 -> v3
// 5. Sub v4, v2 -> v6
// ==========>>==========
// 1. Constant ... -> v3
// 2. Constant ...
// 6. Constant /v1+v2/ -> v5
// 3. ... -> v4
// 4. Sub v3, v1
// 5. Sub v3, v6 -> v6
bool Peepholes::TryOptimizeSubSub(Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (!input1->IsConst() || !inst->HasSingleDataUser()) {
        return false;
    }

    auto single_user = inst->GetSingleDataUser();
    if (single_user->GetOpcode() != Opcode::Sub || !single_user->GetDataInput(1)->IsConst()) {
        return false;
    }

    auto const_near = input1->CastToConstant()->GetImm();
    auto const_far = single_user->GetDataInput(1)->CastToConstant()->GetImm();
    // Overflow is ok
    auto new_const = graph_->CreateConstantInst(const_near + const_far);
    single_user->SetDataInput(1, new_const);
    single_user->SetDataInput(0, input0);
    return true;
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
