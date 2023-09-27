#pragma once

#include <cassert>
#include <istream>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "inst.h"

namespace compiler {

class Graph
{
public:
    Graph() {}

    ~Graph() {
        for (auto inst : all_inst_) {
            assert(inst != nullptr);
            delete inst;
        }
    }

    void SetMethodName(const std::string& name);
    void Dump(std::ostream &out);

#define CREATE_CREATORS(OPCODE)                                    \
    template <typename... Args>                                    \
    auto* Create##OPCODE##Inst(Args&&... args) {                   \
        auto inst = new OPCODE##Inst(std::forward<Args>(args)...); \
        inst->SetId(all_inst_.size());                             \
        all_inst_.push_back(inst);                                 \
        return inst;                                               \
    }

    OPCODE_LIST(CREATE_CREATORS)

#undef CREATE_CREATORS

private:
    std::vector<Inst *> all_inst_;
    std::string name_method_;
};

};
