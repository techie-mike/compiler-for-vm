#include "liveness_analyzer.h"
#include "linear_order.h"
#include "graph.h"

namespace compiler {

void LivenessAnalyzer::Run() {
    ASSERT(graph_->IsInstsPlaced());
    PrepareData();
    BuildLifeNumbers();

    DumpLifeLinearData(std::cerr);
}

void LivenessAnalyzer::PrepareData() {
    auto lo = LinearOrder(graph_);
    lo.Run();
    // Here will be copying of vector
    linear_regions_ = lo.GetVector();

    auto size = linear_regions_.size();
    inst_life_numbers_.reserve(size);
}

void LivenessAnalyzer::BuildLifeNumbers() {
    LifeNumber life_number = 0;
    LinearNumber linear_number = 0;

    for (auto region : linear_regions_) {
        LifeNumber start_block = life_number;
        region->SetLifeNumber(start_block);

        for (Inst *inst = region->GetFirst(); inst != nullptr; inst = inst->GetNext()) {
            inst->SetLinearNumber(linear_number++);
            if (inst->GetOpcode() != Opcode::Phi) {
                life_number += 2;   // +2 reserved for create spill-fill inst. It isn't necessary for Phi inst
            }
            inst->SetLifeNumber(life_number);
        }
        BuildLifeIfJump(region, linear_number, life_number);
        SetRegionLiveRanges(region, LiveRange(start_block, life_number));
    }
}

void LivenessAnalyzer::SetRegionLiveRanges(RegionInst *region, LiveRange &&range) {
    region_live_ranges_[region->GetId()] = range;
}

void LivenessAnalyzer::DumpLifeLinearData(std::ostream &out) {
    bool first = true;
    for (auto region : linear_regions_) {
        if (first) {
            out << "----------------------------\n";
        }
        region->Dump(out);
        PrintLifeLinearData(region, out);
        for (Inst *inst = region->GetFirst(); inst != nullptr; inst = inst->GetNext()) {
            inst->Dump(out);
            PrintLifeLinearData(inst, out);
        }
        if (region->GetOpcode() != Opcode::End) {
            ASSERT(region->GetLast());
            auto inst = region->GetLast();
            inst->Dump(out);
            if (inst->GetOpcode() != Opcode::Jump) {
                PrintLifeLinearData(inst, out);
            }
        }
        out << "----------------------------\n";
        first = false;
    }
}

void LivenessAnalyzer::PrintLifeLinearData(Inst *inst, std::ostream &out) {
    out << "life: " << inst->GetLifeNumber();
    auto opc = inst->GetOpcode();
    if (!inst->IsRegion() && opc != Opcode::Jump) {
        out <<" lin: " << inst->GetLinearNumber();
    }
    out << "\n";
}

void LivenessAnalyzer::BuildLifeIfJump(RegionInst *region, LinearNumber &linear_number, LifeNumber &life_number) {
    if (region->GetLast() == nullptr) {
        return;
    }

    if (region->GetLast()->GetOpcode() == Opcode::If) {
        region->GetLast()->SetLinearNumber(linear_number++);
    }
    ASSERT(region->GetLast()->GetOpcode() == Opcode::Jump);
    life_number += 2;
    region->GetLast()->SetLifeNumber(life_number);
}

}
