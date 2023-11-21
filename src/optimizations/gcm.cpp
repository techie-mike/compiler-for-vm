#include "gcm.h"
#include "analysis/rpo.h"

namespace compiler {

/*
 * TODO: Now write not optimal and not effective algorithm, because that is not main task
*/
void GCM::Run() {
    auto rpo = RpoRegions(graph_);
    rpo.Run();
    for (auto region : rpo.GetVector()) {
        if (region->GetOpcode() == Opcode::End) {
            continue;
        }
        // PHI inst always at the beginning of the region
        for (Inst *fixed_inst = region->GetControlUser();; fixed_inst = fixed_inst->GetControlUser()) {
            auto opc = fixed_inst->GetOpcode();
            if (opc == Opcode::Jump || opc == Opcode::If) {
                PlacingExitFromRegion(fixed_inst, region);
                break;
            }
            PlacingDataInst(fixed_inst, region);
        }
    }
    graph_->SetPlacedInsts();
}

void GCM::PlacingDataInst(Inst *inst, RegionInst *region) {
    if (inst->IsPlaced()) {
        return;
    }

    for (uint32_t i = 0; i < inst->NumDataInputs(); i++) {
        auto input = inst->GetDataInput(i);

        auto opc = input->GetOpcode();
        if (opc == Opcode::Constant || opc == Opcode::Parameter) {
            if (input->IsPlaced()) {
                continue;
            }
            graph_->GetStartRegion()->PushBackInst(input);
            input->SetPlaced();
            continue;
        }

        PlacingDataInst(input, region);
        input->SetPlaced();
        region->PushBackInst(input);
    }
    region->CastToRegion()->PushBackInst(inst);
    inst->SetPlaced();
}

void GCM::PlacingExitFromRegion(Inst *inst, RegionInst *region) {
    auto opc = inst->GetOpcode();
    ASSERT(opc == Opcode::Jump || opc == Opcode::If);
    if (opc == Opcode::Jump) {
        // TODO: Find more good place for this fill
        region->SetExitRegion(inst);
        return;
    }

    if (opc == Opcode::If) {
        // TODO: Find more good place for this fill
        region->SetExitRegion(inst);
        PlacingDataInst(inst->GetDataInput(0), region);
        return;
    }
}

}
