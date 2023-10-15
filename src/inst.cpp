#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

namespace compiler {

bool Inst::IsControlInst() {
    auto opc = GetOpcode();
    return opc == Opcode::Start || opc == Opcode::Region;
}

bool Inst::IsDataInst() {
    return !IsControlInst() && !IsHybridInst();
}

bool Inst::IsHybridInst() {
    auto opc = GetOpcode();
    return opc == Opcode::If || opc == Opcode::Call || opc == Opcode::Return;
}

bool Inst::IsControlInputInHybrid(Inst *inst) {
    ASSERT(inst != nullptr);
    return inst->GetInput(0) == this;
}

bool Inst::IsDataInputInHybrid(Inst *inst) {
    ASSERT(inst != nullptr);
    for (size_t i = 1; i < NumInputs(); i++) {
        if (inst == GetInput(i)) {
            return true;
        }
    }
    return false;
}

void Inst::AddUser(Inst *inst) {
    ASSERT(inst != nullptr);
    // TODO Create more intuitive API
    if (inst->IsControlInst() && IsControlInst() && GetOpcode() != Opcode::Start) {
        *GetUsers().begin() = inst;
        return;
    }
    if (inst->IsHybridInst()) {
        if (IsControlInputInHybrid(inst)) {
            *GetUsers().begin() = inst;
        }
        if (!inst->IsDataInputInHybrid(this)) {
            return;
        }
    }
    auto start_users = inst->IsControlInst() || IsHybridInst() ? ++users_.begin() : users_.begin();
    if (std::find(start_users, users_.end(), inst) != users_.end() && inst!= nullptr) {
        return;
    }
    users_.push_back(inst);
}

void Inst::DeleteUser(Inst *inst) {
    auto it = std::find(users_.begin(), users_.end(), inst);
    ASSERT(it != users_.end());
    users_.erase(it);
}

uint32_t Inst::NumUsers() {
    return users_.size();
}

void CallInst::SetCFGUser(Inst *inst) {
    ASSERT(NumUsers() >= 1);
    ASSERT(inst->GetOpcode() != Opcode::Region);
    // TODO Check that only one CFG user, maybe it should be in graph checker
    *(GetUsers().begin()) = inst;
    inst->SetInput(0, this);
}

Inst *CallInst::GetCFGUser() {
    return *(GetUsers().begin());
}

void Inst::Dump(std::ostream& out) {
    out << std::setw(4) << std::right << std::to_string(GetId()) << std::string(".") << std::setw(4) << std::left << TypeToString(GetType());
    DumpOpcode(out);
    DumpInputs(out);
    DumpUsers(out);
    out << std::endl;
}

void Inst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ");
}

void CompareInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ") << CcToString(GetCC()) << std::string(" ");
}

void Inst::DumpUsers(std::ostream& out) {
    if (NumUsers() == 0) {
        return;
    }
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetUsers()) {
        out << std::string(first ? "" : ", ");
        if (inst == nullptr) {
            out << std::string("NOT_SET");
            first = false;
            continue;
        }
        out << std::string("v") << std::to_string(inst->GetId());
        first = false;
    }
}

void IfInst::DumpUsers(std::ostream &out) {
    ASSERT(NumUsers() == 2);
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetUsers()) {
        out << std::string(first ? "" : ", ");
        if (inst == nullptr) {
            out << std::string("NOT_SET");
            first = false;
            continue;
        }
        out << std::string(first ? "T:" : "F:") << std::string("v") << std::to_string(inst->GetId());
        first = false;
    }
}

void PhiInst::DumpInputs(std::ostream &out) {
    auto region = static_cast<RegionInst *>(GetInput(0));
    if (NumInputs() != region->NumInputs() + 1) {
        std::cerr << "Num inputs in PHI and regions not equal!\n";
        std::exit(1);
    }
    auto it_region = region->GetInputs().begin();
    bool first = true;
    for (auto inst : GetInputs()) {
        if (first) {
            out << std::string("v") << std::to_string(inst->GetId());
            first = false;
            continue;
        }
        out << std::string(", ") << std::string("v") << std::to_string(inst->GetId()) << std::string("(R")
            << std::to_string(static_cast<RegionInst *>(*it_region)->GetId()) << std::string(")");
        first = false;
        it_region++;
    }
}

std::string OpcodeToString(Opcode opc) {
    return OPCODE_NAME.at(static_cast<size_t>(opc));
}

std::string TypeToString(Type type) {
    return TYPE_NAME.at(static_cast<size_t>(type));
}

std::string CcToString(ConditionCode cc) {
    return CC_NAME.at(static_cast<size_t>(cc));
}

}
