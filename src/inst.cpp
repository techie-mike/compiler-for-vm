#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

namespace compiler {

void Inst::AddUser(Inst *inst) {
    if (std::find(users_.begin(), users_.end(), inst) != users_.end()) {
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

void IfInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" [T:->v") << GetTrueBranch()->GetId()
        << std::string(" F:->v") << GetFalseBranch()->GetId() << std::string("] ");
}

void Inst::DumpUsers(std::ostream& out) {
    if (NumUsers() == 0) {
        return;
    }
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : users_) {
        if (inst == nullptr) {
            continue;
        }
        out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
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
