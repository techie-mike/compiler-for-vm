#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <ios>
#include <list>
#include <initializer_list>
#include <string>
#include <iostream>
#include <algorithm>

#include "opcodes.h"
#include "utils/utils.h"

namespace compiler {

/* ======================================================================================
 * Full list of instructions, plus and minus show what support in compiler at the moment:
 * + Constant
 * + Add
 * + Sub
 * + Mul
 * + Div
 * + Region
 * + Start
 * + If
 * + Phi
 * + Call
 * + Return
 * + Compare
 * + Parameter
 * ======================================================================================
*/

using id_t = uint32_t;

std::string OpcodeToString(Opcode opc);
std::string CcToString(ConditionCode cc);
std::string TypeToString(Type type);

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
    virtual ~Inst() = default;

    void Dump(std::ostream& out);

    virtual void DumpOpcode(std::ostream& out);

    id_t GetId() const {
        return id_;
    }

    void SetId(id_t id) {
        id_ = id;
    }

    Type GetType() {
        return type_;
    }

    void SetType(Type type) {
        type_ = type;
    }

    Opcode GetOpcode() const {
        return opc_;
    }

    void SetOpcode(Opcode opc) {
        opc_ = opc;
    }

    void AddUser(Inst *inst);
    void DeleteUser(Inst *inst);

    virtual Inst *GetInput([[maybe_unused]] id_t index) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs";
        std::abort();
    }

    virtual void SetInput([[maybe_unused]] id_t index, [[maybe_unused]] Inst *inst) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs";
        std::abort();
    }

    virtual uint32_t NumInputs() {
        return 0;
    }

    virtual void DumpInputs([[maybe_unused]] std::ostream &out) {};
    virtual void DumpUsers(std::ostream &out);

    uint32_t NumUsers();
    std::list<Inst *> &GetUsers() {
        return users_;
    }

    // Сlassification described in docs/ir.md
    bool IsControlInst();
    bool IsDataInst();
    bool IsHybridInst();

    bool IsControlInputInHybrid(Inst *inst);
    bool IsDataInputInHybrid(Inst *inst);


private:
    id_t id_;
    Opcode opc_;
    Type type_;
    std::list<Inst *> users_;
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
        ASSERT(inputs.size() == N);
        for (size_t i = 0; i < N; i++) {
            inputs_.at(i) = inputs.at(i);
        }
    };

    virtual Inst *GetInput(id_t index) override {
        return inputs_.at(index);
    }

    const std::array<Inst *, N>& GetInputs() const {
        return inputs_;
    }

    virtual void SetInput(id_t index, Inst *inst) override {
        ASSERT(index < N);
        inputs_.at(index) = inst;
        inst->AddUser(this);
    }

    virtual uint32_t NumInputs() override {
        return inputs_.size();
    }

    virtual void DumpInputs(std::ostream &out) override {
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

    std::list<Inst *>& GetInputs() {
        return inputs_;
    }

    void AddInput(Inst *inst) {
        inputs_.push_back(inst);
    }

    void DeleteInput(Inst *inst) {
        auto it = std::find(inputs_.begin(), inputs_.end(), inst);
        ASSERT(it == inputs_.end());
        inputs_.erase(it);
    }

    virtual void SetInput(id_t index, Inst *inst) override {
        ASSERT(index <= NumInputs());
        if (index == NumInputs()) {
            inputs_.push_back(inst);
            inst->AddUser(this);
            return;
        }
        auto it = inputs_.begin();
        for (id_t i = 0; i < index; i++) {
            it++;
        }
        *it = inst;
        inst->AddUser(this);
    }

    virtual Inst *GetInput(id_t index) override {
        auto it = inputs_.begin();
        for (id_t i = 0; i < index; i++) {
            it++;
        }
        return *it;
    }

    virtual uint32_t NumInputs() override {
        return inputs_.size();
    }

    virtual void DumpInputs(std::ostream &out) override {
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

class BinaryOperation : public FixedInputs<2>
{
public:
    BinaryOperation():
        FixedInputs<2>() {}

    BinaryOperation(Opcode opc):
        FixedInputs<2>(opc) {}

    BinaryOperation(Opcode opc, Type type, Inst *input0, Inst *input1):
        FixedInputs<2>(opc, type, {{input0, input1}}) {}
};
class ConstantInst : public Inst, public ImmidiateProperty
{
public:
    ConstantInst():
        Inst(Opcode::Constant, Type::INT64),
        ImmidiateProperty(0) {}

    virtual void DumpInputs(std::ostream &out) override {
        out << std::string("0x") << std::hex << GetImm() << std::dec;
    }
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
        DynamicInputs(Opcode::Region)
    {
        // We reserved user0 for CFG node
        GetUsers().push_back(nullptr);
    }
};

class IfInst : public FixedInputs<2>
{
public:
    // First input is Region
    // Second input is Bool condition value
    IfInst():
        FixedInputs<2>(Opcode::If)
    {
        // True branch (user0)
        GetUsers().push_back(nullptr);
        // False branch (user1)
        GetUsers().push_back(nullptr);
    }

    Inst *GetTrueBranch() {
        return *(GetUsers().begin());
    }

    Inst *GetFalseBranch() {
        return *(++GetUsers().begin());
    }

    void SetTrueBranch(Inst *inst) {
        ASSERT(inst->GetOpcode() == Opcode::Region);
        *(GetUsers().begin()) = inst;
        static_cast<RegionInst *>(inst)->AddInput(this);
    }

    void SetFalseBranch(Inst *inst) {
        ASSERT(inst->GetOpcode() == Opcode::Region);
        *(++GetUsers().begin()) = inst;
        static_cast<RegionInst *>(inst)->AddInput(this);
    }

    virtual void DumpUsers(std::ostream &out) override;
};

class CompareInst : public FixedInputs<2>
{
public:
    CompareInst():
        FixedInputs<2>(Opcode::Compare, Type::BOOL) {};

    void SetCC(ConditionCode cc) {
        cc_ = cc;
    }

    ConditionCode GetCC() {
        return cc_;
    }

    virtual void DumpOpcode(std::ostream& out) override;

private:
    ConditionCode cc_;
};

class PhiInst : public DynamicInputs
{
public:
    PhiInst():
        DynamicInputs(Opcode::Phi) {};

    virtual void DumpInputs(std::ostream &out) override;
};

class ReturnInst : public FixedInputs<2>
{
public:
    ReturnInst():
        FixedInputs<2>(Opcode::Return) {};
};

class ParameterInst : public Inst
{
public:
    ParameterInst():
        Inst() {};

    void SetIndexParam(id_t index) {
        idx_param_ = index;
    }

    id_t GetIndexParam() {
        return idx_param_;
    }

private:
    id_t idx_param_;
};

// Call "NameFunc"
// Inputs:
// 0) CFG element
// 1, ...) - arguments of function
//
// Users:
// 0) CFG element
// 1, ...) - users of return value
class CallInst : public DynamicInputs
{
public:
    CallInst():
        DynamicInputs(Opcode::Call),
        name_func_()
    {
        // We reserved user0 for CFG node
        GetUsers().push_back(nullptr);
        // We reserved input0 for CFG node
        GetInputs().push_back(nullptr);
    };

    void SetCFGUser(Inst *inst);
    Inst *GetCFGUser();

private:
    std::string name_func_;
};

}
