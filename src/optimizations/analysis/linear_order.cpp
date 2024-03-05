#include "linear_order.h"
#include "rpo.h"
#include "analysis.h"
#include "loop_analysis.h"

namespace compiler {

void LinearOrder::Run() {
    if (!graph_->IsInstsPlaced()) {
        UNREACHABLE();
    }

    LoopAnalysis(graph_).Run();
    RecursiveOrder(graph_->GetStartRegion());
}

std::vector<RegionInst *> &LinearOrder::GetVector() {
    return linear_order_;
}

void LinearOrder::RecursiveOrder(RegionInst *region) {
    if (marker_.IsMarked(region)) {
        return;
    }
    // If Region is loop header
    if (region->IsLoopHeader() && !region->GetLoop()->IsIrreducible()) {
        // Two case in this if:
        // 1. If loop is reduceble, check preheaders is visited
        // 2. If loop is irreduceble, don't check something
        if (!region->GetLoop()->IsIrreducible() && !AllPreheadersIsVisited(region)) {
            return;
        }
    } else {
        if (!AllPrevIsVisited(region)) {
            return;
        }
    }

    linear_order_.push_back(region);
    marker_.SetMarker(region);

    if (region->GetOpcode() == Opcode::End) {
        return;
    }

    auto last_inst = region->GetLast();
    if (last_inst->GetOpcode() == Opcode::Jump) {
        RecursiveOrder(last_inst->GetControlUser()->CastToRegion());
        return;
    }
    ASSERT(last_inst->GetOpcode() == Opcode::If);
    // TODO Change condition in IfInst, if false is jump back (region is already visited)
    auto if_inst = last_inst->CastToIf();
    RecursiveOrder(if_inst->GetFalseBranch());
    RecursiveOrder(if_inst->GetTrueBranch());
}

void LinearOrder::AddRegionToOrder(RegionInst *region) {
    linear_order_.push_back(region);
}

bool LinearOrder::AllPrevIsVisited(RegionInst *region) {
    for (id_t i = 0; i < region->NumAllInputs(); i++) {
        auto prev_region = GetRegionByInputRegion(region->GetRegionInput(i));
        if (!marker_.IsMarked(prev_region)) {
            return false;
        }
    }
    return true;
}

bool LinearOrder::AllPreheadersIsVisited(RegionInst *region) {
    auto backedges = region->GetLoop()->GetBackedges();
    for (id_t i = 0; i < region->NumAllInputs(); i++) {
        auto prev_region = GetRegionByInputRegion(region->GetRegionInput(i));
        if (std::find(backedges.begin(), backedges.end(), prev_region) != backedges.end()) {
            continue;
        }
        if (!marker_.IsMarked(prev_region)) {
            return false;
        }
    }
    return true;
}

}
