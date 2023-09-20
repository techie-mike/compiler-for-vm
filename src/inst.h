#pragma once

#include <cstdint>
#include <array>
#include <assert.h>
#include <initializer_list>

#include "opcodes.h"

namespace compiler {


class Inst
{
public:
    Opcode opc;
    Type type;

    Inst():
        opc(Opcode::NONE),
        type(Type::NONE) {};

    Inst(Opcode opc, Type type):
        opc(opc),
        type(type) {};

    virtual void Dump();
};

template <uint32_t N>
class FixedInputs: public Inst
{
public:
    FixedInputs():
        inputs_() {};

    FixedInputs(Opcode opc, Type type, std::array<Inst *, N> inputs) : inputs_(inputs){
        // assert(inputs.size() = N);
        // for (size_t i = 0; i < N; i++) {
        //     inputs_.at(i) = inputs
        // }
    };

    // Do Input class
    Inst *GetInput(uint32_t index) {
        return inputs_.at(index);
    }

    void SetInput(uint32_t index, Inst *inst) {
        inputs_.at(index) = inst;
    }

private:
    std::array<Inst *, N> inputs_;
};

class AddInst : public FixedInputs<2>
{
    AddInst(Type type, Inst *input0, Inst *input1):
        FixedInputs<2>(Opcode::ADD, type, {input0, input1}) {}

};


}
