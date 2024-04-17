#include "checks_elimination.h"
#include "analysis/domtree.h"
#include "analysis/rpo.h"

namespace compiler {

void ChecksElimination::Run() {
    DomTreeSlow(graph_).Run();
    auto rpo_vector = RpoInsts(graph_).Run()->GetVector();

    VisitChecks(rpo_vector);
}

void ChecksElimination::VisitChecks(std::vector<Inst *> &rpo_vector) {
    for (auto inst : rpo_vector) {
        switch (inst->GetOpcode()) {
            case Opcode::NullCheck:
                VisitNullCheck(inst);
                break;
            case Opcode::BoundsCheck:
                VisitBoundCheck(inst);
                break;
            default:
                break;
        }
    }
}

void ChecksElimination::VisitNullCheck(Inst *inst) {
    auto* input = inst->GetDataInput(0);
    for (auto* user : input->GetDataUsers()) {
        if (user->GetOpcode() == Opcode::NullCheck &&
            user != inst &&
            user->IsDominated(inst)) {
            user->ReplaceAllUsers(inst);
        }
    }
}

void ChecksElimination::VisitBoundCheck(Inst *inst) {
    auto* checked_value = inst->GetDataInput(0);
    auto* up_bound = inst->GetDataInput(1);
    for (auto* user : checked_value->GetDataUsers()) {
        if (user->GetOpcode() == Opcode::BoundsCheck &&
            user != inst &&
            user->GetDataInput(1) == up_bound &&
            user->IsDominated(inst)) {
            user->ReplaceAllUsers(inst);
        }
    }
}

}
