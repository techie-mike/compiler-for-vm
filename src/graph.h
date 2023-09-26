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

    /*
     * Functions to create instructions
    */
    auto* CreateConstant() {
        auto inst = new ConstantInst();
        inst->SetId(all_inst_.size());
        all_inst_.push_back(inst);
        return inst;
    }

    template <typename... Args>
    auto* CreateAddInst(Args&&... args) {
        auto inst = new AddInst(std::forward<Args>(args)...);
        inst->SetId(all_inst_.size());
        all_inst_.push_back(inst);
        return inst;
    }

private:
    std::vector<Inst *> all_inst_;
    std::string name_method_;
};

};
