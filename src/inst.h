#pragma once

#include <cstdint>
#include <vector>
#include <list>
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
 * + End
 * + If
 * + Jump
 * + Phi
 * + Call
 * + Return
 * + Compare
 * + Parameter
 * ======================================================================================
*/

class Loop;

using id_t = uint32_t;
using LinearNumber = uint32_t;
using LifeNumber = uint32_t;

std::string OpcodeToString(Opcode opc);
std::string CcToString(ConditionCode cc);
std::string TypeToString(Type type);

class RegionInst;

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

    virtual uint32_t NumAllInputs() {
        return 0;
    }

    uint32_t NumDataInputs() {
        auto num_all = NumAllInputs();
        ASSERT(num_all > 0);
        return HasControlProp() ? num_all - 1 : num_all;
    }

    virtual bool HasControlProp() {
        return false;
    }

    // Next functions not virtual for better perfomance
    void SetControlInput(Inst *inst);
    Inst *GetControlInput();

    void SetControlUser(Inst *inst);
    Inst *GetControlUser();

    void SetDataInput(id_t index, Inst *inst);
    Inst *GetDataInput(id_t index);

    void AddDataUser(Inst *inst);
    void DeleteDataUser(Inst *inst);

    const std::list<Inst *> GetDataUsers();

    virtual void DumpInputs([[maybe_unused]] std::ostream &out) {};
    virtual void DumpUsers(std::ostream &out);

    uint32_t NumDataUsers();
    std::list<Inst *> &GetRawUsers() {
        return users_;
    }

    virtual Inst *GetRawInput([[maybe_unused]] id_t index) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs\n";
        UNREACHABLE();
        return nullptr;
    }

    virtual void SetRawInput([[maybe_unused]] id_t index, [[maybe_unused]] Inst *inst) {
        std::cerr << "Inst with opcode " << OPCODE_NAME[static_cast<size_t>(GetOpcode())] << " don't have inputs\n";
        UNREACHABLE();
        return;
    }

    bool IsRegion() {
        return GetOpcode() == Opcode::Start || GetOpcode() == Opcode::Region || GetOpcode() == Opcode::End;
    }

    void SetPrev(Inst *inst) {
        prev_ = inst;
    }

    Inst *GetPrev() {
        return prev_;
    }

    void SetNext(Inst *inst) {
        next_ = inst;
    }

    Inst *GetNext() {
        return next_;
    }

    RegionInst *CastToRegion();

    bool IsPlaced() {
        return inst_placed_;
    }

    void SetPlaced() {
        inst_placed_ = true;
    }

    void SetLinearNumber(LinearNumber value) {
        linear_number_ = value;
    }

    LinearNumber GetLinearNumber() {
        return linear_number_;
    }

    void SetLifeNumber(LifeNumber value) {
        life_number_ = value;
    }

    LifeNumber GetLifeNumber() {
        return life_number_;
    }

private:
    auto StartIteratorDataUsers() {
        return HasControlProp() ? ++GetRawUsers().begin() : GetRawUsers().begin();
    }

private:
    bool inst_placed_ = false;
    id_t id_;
    LinearNumber linear_number_;
    LifeNumber life_number_;
    Opcode opc_;
    Type type_;

protected:
    Inst *prev_ = nullptr;
    Inst *next_ = nullptr;

private:
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

    virtual uint32_t NumAllInputs() override {
        return inputs_.size();
    }

    virtual void DumpInputs(std::ostream &out) override {
        bool first = true;
        for (auto inst : GetAllInputs()) {
            if (inst == nullptr) {
                continue;
            }
            out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
            first = false;
        }
    }

    const std::array<Inst *, N> GetDataInputs() {
        if (HasControlProp()) {
            return std::array<Inst *, N>(++inputs_.begin(), inputs_.end());
        }
        return inputs_;
    }

    virtual Inst *GetRawInput(id_t index) override {
        return inputs_.at(index);
    }

    virtual void SetRawInput(id_t index, Inst *inst) override {
        ASSERT(index < N);
        inputs_.at(index) = inst;
    }

private:
    const std::array<Inst *, N> &GetAllInputs() {
        return inputs_;
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

    void AddInput(Inst *inst) {
        inputs_.push_back(inst);
    }

    void DeleteInput(Inst *inst) {
        auto it = std::find(inputs_.begin(), inputs_.end(), inst);
        ASSERT(it == inputs_.end());
        inputs_.erase(it);
    }

    virtual uint32_t NumAllInputs() override {
        return inputs_.size();
    }

    virtual void DumpInputs(std::ostream &out) override;

    const std::list<Inst *> GetDataInputs() {
        if (HasControlProp()) {
            return std::list<Inst *>(++inputs_.begin(), inputs_.end());
        }
        return inputs_;
    }

    virtual void SetRawInput(id_t index, Inst *inst) override;
    virtual Inst *GetRawInput(id_t index) override;

private:
    const std::list<Inst *>& GetAllInputs() {
        return inputs_;
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

// Maybe better create bit flag and don't use virtual call
template <typename T>
class ControlProp : public T
{
public:
    using T::T;

    bool HasControlProp() override {
        return true;
    }
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

class RegionInst : public ControlProp<DynamicInputs>
{
public:
    using Base = ControlProp<DynamicInputs>;
    RegionInst():
        Base(Opcode::Region) {}

    virtual ~RegionInst() override;

    Inst *GetRegionInput(uint32_t index) {
        return GetRawInput(index);
    }

    void SetRegionInput(uint32_t index, Inst *inst) {
        [[maybe_unused]] auto opc = inst->GetOpcode();
        ASSERT(opc == Opcode::Start || opc == Opcode::Jump || opc == Opcode::If);
        SetRawInput(index, inst);
    }

    virtual void DumpOpcode(std::ostream& out) override;

    void SetDominator(Inst *inst) {
        dominator_ = inst;
    }

    Inst *GetDominator() {
        return dominator_;
    }

    void AddDominated(Inst *inst) {
        dominated_.push_back(inst);
        static_cast<RegionInst *>(inst)->SetDominator(this);
    }

    std::vector<Inst *> &GetDominated() {
        return dominated_;
    }

    bool IsDominated(Inst *region) {
        ASSERT(region->IsRegion());
        auto res = std::find(dominated_.begin(), dominated_.end(), region);
        return res != dominated_.end();
    }

    Loop *GetLoop() {
        return loop_;
    }

    void SetLoop(Loop *loop) {
        loop_ = loop;
    }

    bool IsLoopHeader();
    void PushBackInst(Inst *inst);
    void PushFrontInst(Inst *inst);

    Inst *GetFirst() {
        return first_;
    }

    Inst *GetLast() {
        return last_;
    }

private:
    void AddFirstInst(Inst *inst);

private:
    Loop *loop_ {nullptr};

    Inst *dominator_ = nullptr;
    // List off insts when they is placed after GCM
    // It allow to reuse already created variable
    Inst *&first_ = next_;
    Inst *&last_ = prev_;
    // --------------------------------------------
    Inst *exit_region_ = nullptr;
    std::vector<Inst *> dominated_;
};

class StartInst : public RegionInst
{
public:
    StartInst():
        RegionInst()
    {
        SetOpcode(Opcode::Start);
    }
};

class EndInst : public RegionInst
{
public:
    EndInst():
        RegionInst()
    {
        SetOpcode(Opcode::End);
    }
};


class IfInst : public ControlProp<FixedInputs<2>>
{
public:
    using Base = ControlProp<FixedInputs<2>>;
    // First input is Region
    // Second input is Bool condition value
    IfInst():
        Base(Opcode::If)
    {
        // True branch (user0)
        GetRawUsers().push_back(nullptr);
        // False branch (user1)
        GetRawUsers().push_back(nullptr);
    }

    Inst *GetTrueBranch() {
        return *(GetRawUsers().begin());
    }

    Inst *GetFalseBranch() {
        return *(++GetRawUsers().begin());
    }

    void SetTrueBranch(Inst *inst) {
        ASSERT(inst->GetOpcode() == Opcode::Region);
        *(GetRawUsers().begin()) = inst;
        static_cast<RegionInst *>(inst)->SetRegionInput(inst->NumAllInputs(), this);
    }

    void SetFalseBranch(Inst *inst) {
        ASSERT(inst->GetOpcode() == Opcode::Region);
        *(++GetRawUsers().begin()) = inst;
        static_cast<RegionInst *>(inst)->SetRegionInput(inst->NumAllInputs(), this);
    }

    virtual void DumpUsers(std::ostream &out) override;
};

class JumpInst : public ControlProp<FixedInputs<1>>
{
public:
    using Base = ControlProp<FixedInputs<1>>;
    JumpInst():
        Base(Opcode::Jump) {}

    void SetJmpTo(Inst *inst) {
        ASSERT(inst->GetOpcode() == Opcode::Region || inst->GetOpcode() == Opcode::End);
        SetControlUser(inst);
        static_cast<RegionInst *>(inst)->SetRegionInput(inst->NumAllInputs(), this);
    }
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

class PhiInst : public ControlProp<DynamicInputs>
{
public:
    using Base = ControlProp<DynamicInputs>;
    PhiInst():
        Base(Opcode::Phi) {};

    virtual void DumpInputs(std::ostream &out) override;
};

class ReturnInst : public ControlProp<FixedInputs<2>>
{
public:
    using Base = ControlProp<FixedInputs<2>>;
    ReturnInst():
        Base(Opcode::Return) {};
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
class CallInst : public ControlProp<DynamicInputs>
{
public:
    using Base = ControlProp<DynamicInputs>;
    CallInst():
        Base(Opcode::Call),
        name_func_() {};

    void SetCFGUser(Inst *inst);
    Inst *GetCFGUser();

private:
    std::string name_func_;
};

}
