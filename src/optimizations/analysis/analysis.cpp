#include "analysis.h"
#include "inst.h"

namespace compiler {

Inst *SkipBodyOfRegion(Inst *inst) {
    ASSERT(inst->GetOpcode() == Opcode::Region || inst->GetOpcode() == Opcode::Start || inst->GetOpcode() == Opcode::End);

    while (inst->GetOpcode() != Opcode::Jump && inst->GetOpcode() != Opcode::If && inst->GetOpcode() != Opcode::End) {
        inst = inst->GetControlUser();
    }
    return inst;
}

}
