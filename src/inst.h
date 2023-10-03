#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <ios>
#include <list>
#include <assert.h>
#include <initializer_list>
#include <string>
#include <iostream>
#include <algorithm>

#include "opcodes.h"

namespace compiler {

/* ======================================================================================
 * Full list of instructions, plus and minus show what support in compiler at the moment:
 * + Constant
 * + Add
 * - Mul
 * + Region
 * + Start
 * + If
 * - Jmp
 * - Phi
 * - Return
 * - Compare
 * - Parameter
 * ======================================================================================
*/

class Inst
{
public:
    Inst():
        opc_(Opcode::NONE),
        type_(Type::NONE) {};

    Inst(Opcode opc):
        opc_(opc),
        type_(Type::NONE) {};

    Inst(Opcode opc, Type type):
        opc_(opc),
        type_(type) {};

    void Dump(std::ostream& out);

    uint32_t GetId() const {
        return id_;
    }

    void SetId(uint32_t id) {
        id_ = id;
    }

    Type GetType() {
        return type_;
    }

    void SetType(Type type) {
        type_ = type;
    }

    Opcode GetOpcode() {
        return opc_;
    }

    virtual Inst *GetInput(uint32_t index) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs";
        std::abort();
    }

    virtual void SetInput(uint32_t index, Inst *inst) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs";
        std::abort();
    }

    virtual uint32_t NumInputs() {
        return 0;
    }

    virtual void DumpInputs(std::ostream &out) const {};


private:
    uint32_t id_;
    Opcode opc_;
    Type type_;
    std::list<Inst *> *user_list;
};

template <uint32_t N>
class FixedInputs: public Inst
{
public:
    FixedInputs():
        inputs_() {};

    FixedInputs(Opcode opc):
        Inst(opc),
        inputs_() {};

    FixedInputs(Opcode opc, Type type):
        Inst(opc, type),
        inputs_() {};

    FixedInputs(Opcode opc, Type type, std::array<Inst *, N> inputs) :
            Inst(opc, type),
            inputs_(inputs) {
        assert(inputs.size() == N);
        for (size_t i = 0; i < N; i++) {
            inputs_.at(i) = inputs.at(i);
        }
    };

    virtual Inst *GetInput(uint32_t index) override {
        return inputs_.at(index);
    }

    const std::array<Inst *, N>& GetInputs() const {
        return inputs_;
    }

    virtual void SetInput(uint32_t index, Inst *inst) override {
        assert(index < N);
        inputs_.at(index) = inst;
    }

    virtual uint32_t NumInputs() override {
        return inputs_.size();
    }

    void DumpInputs(std::ostream &out) const override {
        bool first = true;
        for (auto inst : GetInputs()) {
            if (inst == nullptr) {
                continue;
            }
            out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
            first = false;
        }
    }

private:
    std::array<Inst *, N> inputs_;
};

class DynamicInputs: public Inst
{
public:
    DynamicInputs():
        Inst(),
        inputs_() {};

    DynamicInputs(Opcode opc):
        Inst(opc),
        inputs_() {};

    DynamicInputs(Opcode opc, const std::list<Inst *>& inputs):
            Inst(opc),
            inputs_(inputs) {};

    const std::list<Inst *>& GetInputs() const {
        return inputs_;
    }

    void AddInput(Inst *inst) {
        inputs_.push_back(inst);
    }

    void DeleteInput(Inst *inst) {
        auto it = std::find(inputs_.begin(), inputs_.end(), inst);
        assert(it == inputs_.end());
        inputs_.erase(it);
    }

    virtual void SetInput(uint32_t index, Inst *inst) override {
        auto it = inputs_.begin();
        for (int i = 0; i < index; i++) {
            it++;
        }
        inputs_.insert(it, inst);
    }

    virtual Inst *GetInput(uint32_t index) override {
        auto it = inputs_.begin();
        for (int i = 0; i < index; i++) {
            it++;
        }
        return *it;
    }

    virtual uint32_t NumInputs() override {
        return inputs_.size();
    }

    void DumpInputs(std::ostream &out) const override {
        bool first = true;
        for (auto inst : GetInputs()) {
            if (inst == nullptr) {
                continue;
            }
            out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
            first = false;
        }
    }

private:
    std::list<Inst *> inputs_;
};

using ImmType = int64_t;
class ImmidiateProperty
{
public:

    ImmidiateProperty(ImmType imm):
        imm_(imm) {}

    ImmType GetImm() const {
        return imm_;
    }

    void SetImm(ImmType imm) {
        imm_ = imm;
    }

private:
    ImmType imm_;
};

class AddInst : public FixedInputs<2>
{
public:
    AddInst():
        FixedInputs<2>(Opcode::Add) {}

    AddInst(Type type, Inst *input0, Inst *input1):
        FixedInputs<2>(Opcode::Add, type, {{input0, input1}}) {}

};

class ConstantInst : public Inst, public ImmidiateProperty
{
public:
    ConstantInst():
        Inst(Opcode::Constant, Type::INT64),
        ImmidiateProperty(0) {}

    void DumpInputs(std::ostream &out) const override {
        out << std::string("0x") << std::hex << GetImm() << std::dec;
    }
private:
};

class StartInst : public FixedInputs<1>
{
public:
    StartInst():
        FixedInputs<1>(Opcode::Start) {}
};

class RegionInst : public DynamicInputs
{
public:
    RegionInst():
        DynamicInputs(Opcode::Region) {}

    void SetUser(Inst *inst) {
        user_ = inst;
    }

private:
    Inst *user_;
};

class IfInst : public FixedInputs<2>
{
public:
    // First input is Region
    // Second input is Bool condition value
    IfInst():
        FixedInputs<2>(Opcode::If) {}

    Inst *GetTrueBranch() {
        return GetBranch<BranchWay::True>();
    }

    Inst *GetFalseBranch() {
        return GetBranch<BranchWay::False>();
    }

    void SetTrueBranch(Inst *inst) {
        SetBranch<BranchWay::True>(inst);
    }

    void SetFalseBranch(Inst *inst) {
        SetBranch<BranchWay::False>(inst);
    }

private:
    enum class BranchWay {
        True = 0,
        False
    };

    template<BranchWay V>
    Inst *GetBranch() {
        return branchs_[static_cast<size_t>(V)];
    }

    template<BranchWay V>
    void SetBranch(Inst *inst) {
        assert(inst->GetOpcode() == Opcode::Region);
        static_cast<RegionInst *>(inst)->AddInput(this);
        branchs_[static_cast<size_t>(V)] = inst;
    }

    std::array<Inst *, 2> branchs_;
};

}
