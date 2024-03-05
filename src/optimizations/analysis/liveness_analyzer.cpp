#include "liveness_analyzer.h"
#include "linear_order.h"
#include "graph.h"
#include "loop_analysis.h"

namespace compiler {

void LivenessAnalyzer::Run() {
    ASSERT(graph_->IsInstsPlaced());
    PrepareData();
    BuildLifeNumbers();
    BuildIntervals();
}

void LivenessAnalyzer::PrepareData() {
    // TODO RunPass to eluminate redundant analysis
    auto lo = LinearOrder(graph_);
    lo.Run();
    // Here is copying of vector
    linear_regions_ = lo.GetVector();

    auto size = linear_regions_.size();
    inst_life_numbers_.reserve(size);
}

void LivenessAnalyzer::BuildLifeNumbers() {
    LifeNumber life_number = 0;
    LinearNumber linear_number = 0;

    for (auto region : linear_regions_) {
        FillLifeNumbersInRegionBlock(region, life_number, linear_number);
    }
    num_linear_inst_ = linear_number;
}

void LivenessAnalyzer::FillLifeNumbersInRegionBlock(RegionInst *region, LifeNumber &life_number, LinearNumber &linear_number) {
    LifeNumber start_block = life_number;
    region->SetLifeNumber(start_block);
    for (Inst *inst = region->GetFirst(); inst != nullptr; inst = inst->GetNext()) {
        if (inst->GetOpcode() == Opcode::Jump) {
            inst->SetLifeNumber(life_number);
            break;  // It is last inst in RegionBlock
        }
        inst->SetLinearNumber(linear_number++);
        if (inst->GetOpcode() != Opcode::Phi) {
            life_number += 2;   // +2 reserved for create spill-fill inst. It isn't necessary for Phi inst
        }
        inst->SetLifeNumber(life_number);
    }
    // LifeNumber of the end RegionBlock is more on 2 than value LifeNumber of the last instruction
    if (region->GetOpcode() != Opcode::End) {
        life_number += 2;
    }
    SetRegionLiveRanges(region, LiveRange(start_block, life_number));
}

void LivenessAnalyzer::SetRegionLiveRanges(RegionInst *region, LiveRange &&range) {
    region_live_ranges_[region->GetId()] = range;
}

void LivenessAnalyzer::BuildIntervals() {
    live_intervals_.resize(num_linear_inst_);
    for (LinearNumber i = 0; i < num_linear_inst_; i++) {
        live_intervals_[i].SetLinearNumber(i);
    }

    for (auto it = linear_regions_.rbegin(); it != linear_regions_.rend(); it++) {
        auto region = *it;
        auto live_set = new RegionBlockLiveSet(num_linear_inst_);
        region_block_livesets_[region->GetId()] = live_set;

        // It should be first iteration
        if (region->GetOpcode() == Opcode::End) {
            continue;
        }
        CalcIniteialLiveSet(region);
        ReverseIterateRegionBlock(region);
        ProcessHeaderRegion(region);
    }
}

void LivenessAnalyzer::ProcessHeaderRegion(RegionInst *region) {
    if (!region->IsLoopHeader()) {
        return;
    }
    auto loop = region->GetLoop();
    LifeNumber min_lifenumber = region->GetLifeNumber();
    LifeNumber max_lifenumber = region->GetLifeNumber();
    for (auto body_loop : loop->GetBody()) {
        max_lifenumber = std::max(max_lifenumber, body_loop->GetLifeNumberEndOfRegion());
    }
    UpdateLiveIntervalAllLiveSet(region_block_livesets_[region->GetId()], min_lifenumber, max_lifenumber + 2);
}

void LivenessAnalyzer::CalcIniteialLiveSet(RegionInst *region) {
    for (auto succ_region : region->GetLast()->GetRawUsers()) {
        if (region_block_livesets_[succ_region->GetId()] == nullptr) {
            continue;
        }

        region_block_livesets_[region->GetId()]->Copy(region_block_livesets_[succ_region->GetId()]);
        // TODO Check work with many phi

        if (succ_region->GetOpcode() == Opcode::End) {
            continue;
        }
        for (Inst *i = succ_region->GetControlUser(); i->GetOpcode() == Opcode::Phi; i = i->GetNext()) {
            PhiInst *phi_inst = static_cast<PhiInst *>(i);
            uint32_t index_pred = succ_region->CastToRegion()->GetIndexPredecessor(region->GetLast());
            region_block_livesets_[region->GetId()]->Set(phi_inst->GetDataInput(index_pred)->GetLinearNumber());
        }
    }
    UpdateLiveIntervalAllLiveSet(region_block_livesets_[region->GetId()], region->GetLifeNumber(), region->GetLast()->GetLifeNumber());
}

void LivenessAnalyzer::ReverseIterateRegionBlock(RegionInst *region) {
    auto live_set = region_block_livesets_[region->GetId()];
    for (Inst *inst = region->GetLast(); inst != nullptr; inst = inst->GetPrev()) {
        // Is looking instruction
        if (IsInstWithoutLife(inst)) {
            continue;
        }
        auto inst_linear_number = inst->GetLinearNumber();
        // Return has life_number and linear number, but doesn't have the live interval
        if (HaveLifeInterval(inst)) {
            live_intervals_[inst_linear_number].TrimBegin(inst->GetLifeNumber());
        }
        live_set->Clear(inst_linear_number);

        // Are looking inputs
        if (inst->GetOpcode() == Opcode::Constant || inst->GetOpcode() == Opcode::Parameter) {
            continue;
        }
        auto num_inputs = inst->NumDataInputs();
        for (id_t i = 0; i < num_inputs; i++) {
            auto new_inst = inst->GetDataInput(i);
            live_intervals_[new_inst->GetLinearNumber()].Append(region->GetLifeNumber(), inst->GetLifeNumber());
            live_set->Set(new_inst->GetLinearNumber());
        }
    }
    // TODO Do more intuitive API for work with Phi
    for (Inst *phi = region->GetControlUser(); phi->IsPhi(); phi = phi->GetControlUser()) {
        live_set->Clear(phi->GetLinearNumber());
    }
}


void LivenessAnalyzer::DumpLifeLinearData(std::ostream &out) {
    bool first = true;
    for (auto region : linear_regions_) {
        if (first) {
            out << "----------------------------\n";
        }
        out << "BB begin life:" << region->GetLifeNumber() << std::endl;
        region->Dump(out);

        for (Inst *inst = region->GetFirst(); inst != nullptr; inst = inst->GetNext()) {
            inst->Dump(out);
            PrintLifeLinearData(inst, out);
        }

        if (region->GetOpcode() != Opcode::End) {
            out << "Block live range: [" << region_live_ranges_[region->GetId()].GetBegin() << ", "
                                         << region_live_ranges_[region->GetId()].GetEnd() << ")" << std::endl;
        }
        out << "----------------------------\n";
        first = false;
    }
}

void LivenessAnalyzer::PrintLifeLinearData(Inst *inst, std::ostream &out) {
    auto opc = inst->GetOpcode();
    if (!inst->IsRegion() && opc != Opcode::Jump) {
        out << "       life: " << inst->GetLifeNumber();
        out <<" lin: " << inst->GetLinearNumber();
        out << "\n";
    }
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

void LivenessAnalyzer::UpdateLiveIntervalAllLiveSet(RegionBlockLiveSet* live_set, LifeNumber begin, LifeNumber end) {
    for (LinearNumber i = 0; i < live_set->Size(); i++) {
        if (live_set->IsSet(i)) {
            live_intervals_[i].Append(begin, end);
        }
    }
}

void LivenessAnalyzer::DumpIntervals() {
    for (LinearNumber i = 0; i < num_linear_inst_; i++) {
        auto live_interval = live_intervals_[i];
        std::cout << "Interval:" << i << " [" << live_interval.GetBegin() << ", " << live_interval.GetEnd() << ")" << std::endl;
    }
}

}
