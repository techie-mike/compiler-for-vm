#pragma once

#include "inst.h"
#include "graph.h"

namespace compiler {

class Marker
{
public:
    Marker(Graph *graph):
        array_(new bool [graph->GetNumInsts()]),
        num_insts_(graph->GetNumInsts())
    {
        Clear();
    }

    ~Marker() {
        delete [] array_;
    }

    void SetMarker(Inst *inst) {
        ASSERT(inst != nullptr);
        array_[inst->GetId()] = true;
    }

    bool IsMarked(Inst *inst) {
        ASSERT(inst != nullptr);
        return array_[inst->GetId()];
    }

    void Clear() {
        for (size_t i = 0; i < num_insts_; i++) {
            array_[i] = 0;
        }
    }

private:
    // TODO: Change to bitset
    bool *array_;
    uint32_t num_insts_;
};

}
