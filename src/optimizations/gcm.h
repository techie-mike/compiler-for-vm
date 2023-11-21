#pragma once

#include "graph.h"

namespace compiler {


// After graph can have less instrutions than was before, because during Global Code Motion
// not added unused instructions in regions
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
