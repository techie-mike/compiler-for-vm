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
            if (inst == nullptr) {
                continue;
            }
            delete inst;
        }
    }

    void SetMethodName(const std::string& name);
    std::string GetMethodName() const;

    void Dump(std::ostream &out);

#define CREATE_CREATORS(OPCODE, BASE, ...)                                                  \
    template <typename... Args>                                                             \
    auto *Create##OPCODE##Inst(Args&&... args) {                                            \
        auto inst = new BASE(std::forward<Args>(args)...);                                  \
        /* TODO - Bad API, perhaps there is a better way */                                 \
        ASSERT(inst->GetOpcode() == Opcode::OPCODE || inst->GetOpcode() == Opcode::NONE);   \
        inst->SetOpcode(Opcode::OPCODE);                                                    \
        inst->SetId(all_inst_.size());                                                      \
        all_inst_.push_back(inst);                                                          \
        return inst;                                                                        \
    }

    OPCODE_LIST(CREATE_CREATORS)

#undef CREATE_CREATORS

    Inst *CreateClearInstByOpcode(Opcode opc);

    template <Opcode T>
    auto* CreateInstByIndex(id_t index) {
        if (!unit_test_mode_) {
            std::cerr << "Function only for unit tests\n";
            std::exit(1);
        }
        if (index < GetNumInsts() && GetInstByIndex(index) != nullptr) {
            std::cerr << "Inst with index " << index << " already exist\n";
            std::exit(1);
        }
        auto inst = CreateClearInstByOpcode(T);
        inst->SetId(index);
        if (index >= all_inst_.size()) {
            all_inst_.resize(index + 1);
        }
        all_inst_.at(index) = inst;
        return inst;
    }

    Inst *GetInstByIndex(id_t index) {
        return all_inst_.at(index);
    }

    void SetUnitTestMode() {
        unit_test_mode_ = true;
    }

    size_t GetNumInsts() {
        return all_inst_.size();
    }

private:
    std::vector<Inst *> all_inst_;
    std::string name_method_;
    bool unit_test_mode_;
};

};
