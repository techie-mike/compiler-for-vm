#include "inst.h"
#include "opcodes.h"
#include <string>
#include <iomanip>

namespace compiler {

void Inst::Dump(std::ostream& out) {
    out << std::setw(4) << std::to_string(GetId()) << std::string(".");
    out << std::setw(10) << OPCODE_NAME.at(static_cast<size_t>(GetOpcode())) << std::string(" ");
    DumpInputs(out);
    out << std::endl;
}

}
