#pragma once

#include "inst.h"
#include "marker.h"

namespace compiler {

class Graph;

class LinearOrder {
public:
    LinearOrder(Graph *graph):
        graph_(graph),
        marker_(graph) {};

    void Run();

    std::vector<RegionInst *> &GetVector();

private:
    void RecursiveOrder(RegionInst *region);
    void AddRegionToOrder(RegionInst *region);
    bool AllPrevIsVisited(RegionInst *region);
    bool AllPreheadersIsVisited(RegionInst *region);

private:
    Graph *graph_;
    Marker marker_;
    std::vector<RegionInst *> linear_order_;
};

}
