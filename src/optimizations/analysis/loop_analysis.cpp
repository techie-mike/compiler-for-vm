#include "loop_analysis.h"
#include "marker.h"
#include "analysis.h"

#include "domtree.h"
#include "rpo.h"

namespace compiler {

void LoopAnalysis::Run() {
    auto start = static_cast<RegionInst *>(graph_->GetInstByIndex(0));

    DomTreeSlow(graph_).Run();
    DFSRegion(start, nullptr);
    CreateRootLoop();
    CompliteLoops();
    SetLoopProperties(graph_->GetRootLoop(), 0);
}

void LoopAnalysis::DFSRegion(RegionInst *region, RegionInst *prev_region) {
    // Process inst if already visited it on owr way from root
    if (m_trace_.TrySetMarker(region)) {
        ProcessNewBackEdge(region, prev_region);
    }

    if (m_visited_.TrySetMarker(region)) {
        m_trace_.SetMarker(region, false);
        return;
    }

    auto control = SkipBodyOfRegion(region);
    for (auto new_region : control->GetRawUsers()) {
        ASSERT(new_region->IsRegion());
        DFSRegion(static_cast<RegionInst *>(new_region), region);
    }

    m_trace_.SetMarker(region, false);
}

Loop *LoopAnalysis::CreateLoop(RegionInst *region) {
    ASSERT(region->GetOpcode() == Opcode::Region);
    auto loop = new Loop;
    graph_->IncNumLoops();
    region->SetLoop(loop);
    loop->SetId(graph_->GetNumLoops());
    loop->SetHeader(region);
    loop->AddRegion(region);
    return loop;
}

void LoopAnalysis::ProcessNewBackEdge(RegionInst *header, RegionInst *backedge) {
    auto loop = header->GetLoop();
    if (loop == nullptr) {
        loop = CreateLoop(header);
    }

    loop->AddBackedge(backedge);
    if (!header->IsDominated(backedge)) {
        loop->SetIrreducibleLoop();
    }
}

void LoopAnalysis::CompliteLoops() {
    auto rpo = RpoRegions(graph_);
    rpo.Run();
    auto &rpo_regions = rpo.GetVector();

    for (auto it = rpo_regions.rbegin(); it != rpo_regions.rend(); it++) {
        auto region = *it;
        if (region->GetLoop() == nullptr || !region->IsLoopHeader()) {
            continue;
        }
        auto loop = region->GetLoop();
        if (loop->IsIrreducible()) {
            for (auto backedge : loop->GetBackedges()) {
                if (backedge->GetLoop() != loop) {
                    loop->AddRegion(backedge);
                }
            }
        } else {
            m_visited_.Clear();
            m_visited_.SetMarker(region);

            for (auto backedge : loop->GetBackedges()) {
                FillLoop(loop, backedge);
            }
        }
    }

    // Connect free regions to root loop
    auto root_loop = graph_->GetRootLoop();
    for (auto region : rpo_regions) {
        // If region isn't located in some loop
        if (region->GetLoop() == nullptr) {
            root_loop->AddRegion(region);
        // If loop don't have outer loop
        } else if (region->GetLoop()->GetOuterLoop() == nullptr) {
            region->GetLoop()->SetOuterLoop(root_loop);
            root_loop->AppendInnerLoop(region->GetLoop());
        }
    }
}

void LoopAnalysis::FillLoop(Loop *loop, RegionInst *region) {
    if (m_visited_.TrySetMarker(region)) {
        return;
    }
    if (region->GetLoop() == nullptr) {
        loop->AddRegion(region);
    } else if (region->GetLoop()->GetHeader() != loop->GetHeader()) {
        if (region->GetLoop()->GetOuterLoop() == nullptr) {
            region->GetLoop()->SetOuterLoop(loop);
            loop->AppendInnerLoop(region->GetLoop());
        }
    }

    auto num_inputs = region->NumAllInputs();
    for (uint32_t i = 0; i < num_inputs; i++) {
        FillLoop(loop, GetRegionByInputRegion(region->GetRegionInput(i)));
    }
}

void LoopAnalysis::CreateRootLoop() {
    ASSERT(graph_->GetRootLoop() == nullptr);
    auto root_loop = new Loop;
    root_loop->SetId(0);
    graph_->SetRootLoop(root_loop);
}

void LoopAnalysis::SetLoopProperties(Loop *loop, uint32_t depth) {
    // TODO Add check infinity loop
    loop->SetDepth(depth);
    depth++;
    for (auto inner_loop : loop->GetInnerLoops()) {
        SetLoopProperties(inner_loop, depth);
    }
}

}
