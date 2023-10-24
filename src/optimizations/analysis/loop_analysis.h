#pragma once

#include "graph.h"
#include "marker.h"

namespace compiler {

class Loop
{
public:

    ~Loop() {
        for (auto region : body_) {
            region->SetLoop(nullptr);
        }
    }

    RegionInst *GetHeader() {
        return header_;
    }

    void SetHeader(RegionInst *region) {
        header_ = region;
    }

    uint32_t GetId() {
        return id_loop_;
    }

    void SetId(uint32_t id) {
        id_loop_ = id;
    }

    bool IsIrreducible() {
        return irreducible_;
    }

    void SetIrreducibleLoop() {
        irreducible_ = true;
    }

    std::vector<RegionInst *> &GetBackedges() {
        return backedge_;
    }

    void AddBackedge(RegionInst *region) {
        ASSERT(std::find(backedge_.begin(), backedge_.end(), region) == backedge_.end())
        backedge_.push_back(region);
    }

    void AddRegion(RegionInst *region) {
        ASSERT(std::find(body_.begin(), body_.end(),  region) == body_.end());
        region->SetLoop(this);
        body_.push_back(region);
    }

    void SetOuterLoop(Loop *loop) {
        outer_loop_ = loop;
    }

    Loop *GetOuterLoop() {
        return outer_loop_;
    }

    void AppendInnerLoop(Loop *loop) {
        inner_loops_.push_back(loop);
    }

    const std::vector<Loop *> &GetInnerLoops() {
        return inner_loops_;
    }

    void SetDepth(uint32_t depth) {
        depth_ = depth;
    }

    uint32_t GetDepth() {
        return depth_;
    }

private:
    // TODO: Create bitSet instead of bool
    RegionInst *header_ {nullptr};
    bool irreducible_ = false;
    uint32_t id_loop_ = 0;
    uint32_t depth_ = 0;
    Loop *outer_loop_ = nullptr;
    std::vector<RegionInst *> backedge_;
    std::vector<RegionInst *> body_;
    std::vector<Loop *> inner_loops_;
};

class LoopAnalysis
{
public:
    LoopAnalysis(Graph *graph):
        graph_(graph),
        m_visited_(graph),
        m_trace_(graph) {}

    void Run();
    void DFSRegion(RegionInst *region, RegionInst *prev_region);
    Loop *CreateLoop(RegionInst *region);
    void ProcessNewBackEdge(RegionInst *header, RegionInst *backedge);
    void SetLoopProperties(Loop *loop, uint32_t depth);

    void CompliteLoops();
    void FillLoop(Loop *loop, RegionInst *region);
    void CreateRootLoop();

private:
    Graph *graph_;
    Marker m_visited_;
    Marker m_trace_;
    uint32_t num_loops_ = 0;
};

}
