#include "constant_folding.h"
#include "graph.h"

namespace compiler {

bool ConstFoldingBinaryOp(Graph *graph, Inst *inst) {
    auto input0 = inst->GetDataInput(0);
    auto input1 = inst->GetDataInput(1);

    if (!input0->IsConst() || !input1->IsConst()) {
        return false;
    }

    Inst *new_const = nullptr;
    auto imm0 = input0->CastToConstant()->GetImm();
    auto imm1 = input1->CastToConstant()->GetImm();

    switch (inst->GetOpcode()) {
        case Opcode::Sub:
            new_const = graph->CreateConstantInst(imm0 - imm1);
            break;
        case Opcode::Shl:
            new_const = graph->CreateConstantInst(imm0 << imm1);
            break;
        case Opcode::Or:
            new_const = graph->CreateConstantInst(imm0 | imm1);
            break;
        default:
            UNREACHABLE();
    }
    new_const->ReplaceDataUsers(inst);
    return true;
}

}
