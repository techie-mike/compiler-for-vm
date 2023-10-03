#pragma once

#include "graph.h"
#include <gtest/gtest.h>

namespace compiler {

class GraphComparator
{
public:
    GraphComparator(Graph *left, Graph *right):
        left_(left), right_(right) {}

    void Compare();

private:
    void CompareInstructions(Inst *left, Inst *right);
    void CompareInputs(Inst *left, Inst *right);

private:
    Graph *left_;
    Graph *right_;
};

}
