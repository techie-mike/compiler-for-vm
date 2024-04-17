#pragma once

#include "graph.h"
#include "marker.h"

namespace compiler {

class RpoRegions
{
public:
    RpoRegions(Graph *graph);

    void Run();
    std::vector<RegionInst *> &GetVector();

private:
    void DFSRegions(Inst *inst, Marker &marker);
    void AddInstInVector(Inst *inst);
    std::vector<RegionInst *> rpo_regions_;
    Graph *graph_;
};

class RpoInsts
{
public:
    RpoInsts(Graph *graph);

    RpoInsts* Run();
    void DFSInsts(Inst *inst, Marker &marker);
    std::vector<Inst *> &GetVector();

private:
    void AddInstInVector(Inst *inst);
    std::vector<Inst *> rpo_insts_;
    Graph *graph_;
};

}
