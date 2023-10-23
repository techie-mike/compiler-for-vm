#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

namespace compiler {

void Inst::SetControlInput(Inst *inst) {
    ASSERT(inst != nullptr);
    ASSERT(HasControlProp());
    SetRawInput(0, inst);
    inst->SetControlUser(this);
}

Inst *Inst::GetControlInput() {
    ASSERT(HasControlProp());
    return GetRawInput(0);
}

void Inst::SetControlUser(Inst *inst) {
    ASSERT(HasControlProp());
    if (GetUsers().size() == 0) {
        GetUsers().push_back(inst);
        return;
    }
    *GetUsers().begin() = inst;
}

Inst *Inst::GetControlUser() {
    ASSERT(HasControlProp());
    ASSERT(GetUsers().size() > 0);
    return *GetUsers().begin();
}

void Inst::SetDataInput(id_t index, Inst *inst) {
    ASSERT(inst != nullptr);
    ASSERT(index <= NumDataInputs());
    if (HasControlProp()) {
        index++;
    }
    SetRawInput(index, inst);
    inst->AddDataUser(this);
}

Inst *Inst::GetDataInput(id_t index) {
    if (HasControlProp()) {
        index++;
    }
    return GetRawInput(index);
}

const std::list<Inst *> Inst::GetDataUsers() {
    return std::list<Inst *>(StartIteratorDataUsers(), GetUsers().end());
}

void Inst::AddDataUser(Inst *inst) {
    if (HasControlProp() && GetUsers().size() == 0) {
        GetUsers().push_back(nullptr);
    }
    auto it = std::find(StartIteratorDataUsers(), GetUsers().end(), inst);
    if (it != GetUsers().end()) {
        return;
    }
    GetUsers().push_back(inst);
}

void Inst::DeleteDataUser(Inst *inst) {
    auto it = std::find(StartIteratorDataUsers(), GetUsers().end(), inst);
    ASSERT(it != GetUsers().end());
    users_.erase(it);
}

uint32_t Inst::NumDataUsers() {
    return HasControlProp() ? GetUsers().size() - 1 : GetUsers().size();
}

void DynamicInputs::SetRawInput(id_t index, Inst *inst) {
    ASSERT(index <= NumAllInputs());
    if (index == NumAllInputs()) {
        inputs_.push_back(inst);
        return;
    }
    auto it = inputs_.begin();
    for (id_t i = 0; i < index; i++) {
        it++;
    }
    *it = inst;
}

Inst *DynamicInputs::GetRawInput(id_t index) {
    auto it = inputs_.begin();
    for (id_t i = 0; i < index; i++) {
        it++;
    }
    return *it;
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
    if (GetUsers().size() == 0) {
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
    ASSERT(GetUsers().size() == 2);
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
    auto region = static_cast<RegionInst *>(GetControlInput());
    if (NumDataInputs() != region->NumAllInputs()) {
        std::cerr << "Num inputs in PHI and regions not equal!\n";
        std::exit(1);
    }
    uint32_t i = 0;
    out << std::string("v") << std::to_string(GetControlInput()->GetId());
    for (auto inst : GetDataInputs()) {
        out << std::string(", ") << std::string("v") << std::to_string(inst->GetId()) << std::string("(R")
            << std::to_string((region->GetRegionInput(i))->GetId()) << std::string(")");
        i++;
    }
}

void DynamicInputs::DumpInputs(std::ostream &out) {
        bool first = true;
        for (auto inst : GetAllInputs()) {
            if (inst == nullptr) {
                continue;
            }
            out << std::string(first ? "" : ", ") << std::string("v") << std::to_string(inst->GetId());
            first = false;
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
