#pragma once

#include "inst.h"

namespace compiler {

class LiveInterval;

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
    void DumpDomTree(std::ostream &out);

#define CREATE_CREATORS(OPCODE, BASE)                                                       \
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

    INST_OPCODE_LIST(CREATE_CREATORS)

#undef CREATE_CREATORS

// TODO: fix copying of define
#define CREATE_CREATORS_REGIONS(OPCODE, BASE)                                               \
    template <typename... Args>                                                             \
    auto *Create##OPCODE##Inst(Args&&... args) {                                            \
        auto inst = new BASE(std::forward<Args>(args)...);                                  \
        /* TODO - Bad API, perhaps there is a better way */                                 \
        ASSERT(inst->GetOpcode() == Opcode::OPCODE || inst->GetOpcode() == Opcode::NONE);   \
        inst->SetOpcode(Opcode::OPCODE);                                                    \
        inst->SetId(all_inst_.size());                                                      \
        all_inst_.push_back(inst);                                                          \
        all_regions_.push_back(inst);                                                       \
        return inst;                                                                        \
    }

    REGIONS_OPCODE_LIST(CREATE_CREATORS_REGIONS)

#undef CREATE_CREATORS_REGIONS

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

    const std::vector<Inst *> &GetAllInsts() {
        return all_inst_;
    }

    uint32_t GetNumLoops() {
        return num_loops_;
    }

    void IncNumLoops() {
        num_loops_++;
    }

    void DecNumLoops() {
        num_loops_--;
    }

    Loop *GetRootLoop() {
        return root_loop_;
    }

    void SetRootLoop(Loop *loop) {
        root_loop_ = loop;
    }

    void DumpPlacedInsts(std::ostream &out);

    RegionInst *GetStartRegion() {
        return GetInstByIndex(0)->CastToRegion();
    }

    void SetInstsPlaced() {
        insts_placed_ = true;
    }

    bool IsInstsPlaced() {
        return insts_placed_;
    }

private:
    bool unit_test_mode_ = false;
    bool insts_placed_ = false;
    uint32_t num_loops_ = 0;
    Loop *root_loop_ = nullptr;
    std::string name_method_;
    std::vector<Inst *> all_inst_;
    std::vector<RegionInst *> all_regions_;
};

}
