#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

#include "optimizations/analysis/loop_analysis.h"

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
    if (GetRawUsers().size() == 0) {
        GetRawUsers().push_back(inst);
        return;
    }
    *GetRawUsers().begin() = inst;
}

Inst *Inst::GetControlUser() {
    ASSERT(HasControlProp());
    ASSERT(GetRawUsers().size() > 0);
    return *GetRawUsers().begin();
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
    return std::list<Inst *>(StartIteratorDataUsers(), GetRawUsers().end());
}

void Inst::AddDataUser(Inst *inst) {
    if (HasControlProp() && GetRawUsers().size() == 0) {
        GetRawUsers().push_back(nullptr);
    }
    auto it = std::find(StartIteratorDataUsers(), GetRawUsers().end(), inst);
    if (it != GetRawUsers().end()) {
        return;
    }
    GetRawUsers().push_back(inst);
}

void Inst::DeleteDataUser(Inst *inst) {
    auto it = std::find(StartIteratorDataUsers(), GetRawUsers().end(), inst);
    ASSERT(it != GetRawUsers().end());
    users_.erase(it);
}

uint32_t Inst::NumDataUsers() {
    return HasControlProp() ? GetRawUsers().size() - 1 : GetRawUsers().size();
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

void RegionInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << std::left << OpcodeToString(GetOpcode()) << std::string(" ");
    if (loop_ == nullptr) {
        return;
    }

    out << std::string("[Loop:");
    if (GetLoop()->GetId() == 0) {
        out << "root";
    } else {
        out << GetLoop()->GetId() << ", Depth:" << GetLoop()->GetDepth();
    }
    if (loop_->GetHeader() == this) {
        out << std::string(", Header");
    }
    auto backedges = loop_->GetBackedges();
    if (std::find(backedges.begin(), backedges.end(), this) != backedges.end()) {
        out << std::string(", Backedge");
    }
    if (loop_->IsIrreducible()) {
        out << std::string(", Irreducible");
    }
    out << std::string("] ");
}

void Inst::DumpUsers(std::ostream& out) {
    if (GetRawUsers().size() == 0) {
        return;
    }
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetRawUsers()) {
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
    ASSERT(GetRawUsers().size() == 2);
    out << std::string(" -> ");
    bool first = true;
    for (auto inst : GetRawUsers()) {
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

RegionInst::~RegionInst() {
    if (loop_ != nullptr) {
        delete loop_;
    }
}

bool RegionInst::IsLoopHeader() {
    return GetLoop()->GetHeader() == this;
}

void RegionInst::AddFirstInst(Inst *inst) {
    ASSERT(first_ == nullptr && last_ == nullptr);
    first_ = inst;
    last_ = inst;
}

void RegionInst::PushBackInst(Inst *inst) {
    inst->SetPlaced();
    if (first_ == nullptr) {
        AddFirstInst(inst);
        return;
    }

    last_->SetNext(inst);
    inst->SetPrev(last_);
    last_ = inst;
}

void RegionInst::PushFrontInst(Inst *inst) {
    inst->SetPlaced();
    if (first_ == nullptr) {
        AddFirstInst(inst);
        return;
    }

    first_->SetPrev(inst);
    inst->SetNext(first_);
    first_ = inst;
}

RegionInst *Inst::CastToRegion() {
    ASSERT(IsRegion());
    return static_cast<RegionInst *>(this);
}

IfInst *Inst::CastToIf() {
    ASSERT(GetOpcode() == Opcode::If);
    return static_cast<IfInst *>(this);
}

}
