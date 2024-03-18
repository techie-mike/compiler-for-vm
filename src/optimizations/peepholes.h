#pragma once

#include "graph.h"

namespace compiler {

class Peepholes {
public:
    Peepholes(Graph *graph):
        graph_(graph) {}

    void Run();

private:
    void AnalysisInst(Inst *inst);

    // Process instruction
    void VisitSub(Inst *inst);
    void VisitShl(Inst *inst);
    void VisitOr(Inst *inst);

    // Cases of optimization
    bool TryOptimizeSubZero(Inst *inst);
    bool TryOptimizeSubSub(Inst *inst);
    bool TryOptimizeShlAfterShr(Inst *inst);
    bool TryOptimizeOrZero(Inst *inst);

private:
    Graph *graph_;
};

}