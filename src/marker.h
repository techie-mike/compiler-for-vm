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

    void SetMarker(Inst *inst, bool value=true) {
        ASSERT(inst != nullptr);
        array_[inst->GetId()] = value;
    }

    bool IsMarked(Inst *inst) {
        ASSERT(inst != nullptr);
        return array_[inst->GetId()];
    }

    bool TrySetMarker(Inst *inst) {
        ASSERT(inst != nullptr);
        if (array_[inst->GetId()]) {
            return true;
        }
        array_[inst->GetId()] = true;
        return false;
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
