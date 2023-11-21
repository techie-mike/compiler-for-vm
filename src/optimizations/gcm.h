#pragma once

#include "graph.h"

namespace compiler {

class GCM
{
public:
    GCM(Graph *graph):
        graph_(graph) {};

    void Run();

private:
    void PlacingDataInst(Inst *inst, RegionInst *region);
    void PlacingExitFromRegion(Inst *inst, RegionInst *region);

private:
    Graph *graph_;
};

}
