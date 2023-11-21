#pragma once

#include "inst.h"

namespace compiler {

class Graph;

class LinearOrder {
public:
    LinearOrder(Graph *graph):
        graph_(graph) {};

    void Run();

    std::vector<RegionInst *> &GetVector();

private:
    Graph *graph_;
    std::vector<RegionInst *> linear_order_;
};

}
