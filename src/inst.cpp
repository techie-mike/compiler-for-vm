#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

namespace compiler {

void Inst::Dump(std::ostream& out) {
    out << std::setw(4) << std::to_string(GetId()) << std::string(".");
    DumpOpcode(out);
    DumpInputs(out);
    out << std::endl;
}

void Inst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << OpcodeToString(GetOpcode()) << std::string(" ");
}

void CompareInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << OpcodeToString(GetOpcode()) << std::string(" ") << CcToString(GetCC()) << std::string(" ");
}

void IfInst::DumpOpcode(std::ostream& out) {
    out << std::setw(10) << OpcodeToString(GetOpcode()) << std::string(" [T:->v") << GetTrueBranch()->GetId()
        << std::string(" F:->v") << GetFalseBranch()->GetId() << std::string("] ");
}


std::string OpcodeToString(Opcode opc) {
    return OPCODE_NAME.at(static_cast<size_t>(opc));
}

std::string CcToString(ConditionCode cc) {
    return CC_NAME.at(static_cast<size_t>(cc));
}

}
