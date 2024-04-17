#pragma once

#include "vector"

namespace compiler {

class Graph;
class Inst;

class ChecksElimination
{
public:
    ChecksElimination(Graph *graph):
        graph_(graph) {};

    void Run();

private:
    void VisitChecks(std::vector<Inst *> &rpo);
    void VisitNullCheck(Inst *inst);
    void VisitBoundCheck(Inst *inst);

private:
    Graph *graph_;
};

}
