#pragma once

#include "graph.h"
#include "marker.h"
#include "analysis.h"

namespace compiler {

class DomTreeSlow
{
public:
    DomTreeSlow(Graph *graph):
        graph_(graph) {};
    void Run();

private:
    void DFSRegions(Inst *region, std::vector<Inst *> &found, Marker &marker);
    Graph *graph_;
};

}
