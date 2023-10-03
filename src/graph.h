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
    std::string GetMethodName() const;

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

    template <typename T>
    auto* CreateInstByIndex(uint32_t index) {
        if (!unit_test_mode_) {
            std::cerr << "Function only for unit tests\n";
            std::abort();
        }
        auto inst = new T();
        inst->SetId(index);
        if (index >= all_inst_.size()) {
            all_inst_.resize(index + 1);
        }
        all_inst_.at(index) = inst;
        return inst;
    }

    Inst *GetInstByIndex(uint32_t index) {
        return all_inst_.at(index);
    }

    void SetUnitTestMode() {
        unit_test_mode_ = true;
    }

    uint32_t GetNumInsts() {
        return all_inst_.size();
    }

private:
    std::vector<Inst *> all_inst_;
    std::string name_method_;
    bool unit_test_mode_;
};

};
