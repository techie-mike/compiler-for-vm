#pragma once

#include <cstdint>
#include <array>
#include <ios>
#include <list>
#include <assert.h>
#include <initializer_list>
#include <string>
#include <iostream>

#include "opcodes.h"

namespace compiler {

/* Full list of instructions, plus and minus show what support in compiler at the moment:
 * + Constant
 * + Add
 * - Mul
 * - Region
 * - Start
 * - If
 * - Jmp
 * - Phi
 * - Return
 * - Compare
 * - Parameter
*/

class Inst
{
public:
    Inst():
        opc_(Opcode::NONE),
        type_(Type::NONE) {};

    Inst(Opcode opc, Type type):
        opc_(opc),
        type_(type) {};

    // virtual ~Inst() = default;

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

    FixedInputs(Opcode opc, Type type, std::array<Inst *, N> inputs) :
            Inst(opc, type),
            inputs_(inputs) {
        assert(inputs.size() == N);
        for (size_t i = 0; i < N; i++) {
            inputs_.at(i) = inputs.at(i);
        }
    };

    Inst *GetInput(uint32_t index) {
        return inputs_.at(index);
    }

    const std::array<Inst *, N>& GetInputs() const {
        return inputs_;
    }

    void SetInput(uint32_t index, Inst *inst) {
        inputs_.at(index) = inst;
    }

    void DumpInputs(std::ostream &out) const override {
        bool first = true;
        for (auto inst : GetInputs()) {
            out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
            first = false;
        }
    }

private:
    std::array<Inst *, N> inputs_;
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

}
