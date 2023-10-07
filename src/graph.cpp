#include "graph.h"
#include <iterator>
#include <ostream>

namespace compiler {

void Graph::Dump(std::ostream &out)
{
    out << "Method: " << name_method_ << std::endl;
    out << "Instructions:" << std::endl;
    for (auto inst : all_inst_) {
        inst->Dump(out);
    }
}

void Graph::SetMethodName(const std::string& name)
{
    name_method_ = name;
}

std::string Graph::GetMethodName() const
{
    return name_method_;
}

Inst *Graph::CreateClearInstByOpcode(Opcode opc) {
    switch(opc) {

#define CREATER_BY_OPCODE(OPCODE, BASE, ...)                                                    \
        case Opcode::OPCODE: {                                                                  \
            auto *inst = new BASE();                                                            \
            assert(inst->GetOpcode() == Opcode::OPCODE || inst->GetOpcode() == Opcode::NONE);   \
            inst->SetOpcode(Opcode::OPCODE);                                                    \
            return inst;                                                                        \
        }                                                                                       \

OPCODE_LIST(CREATER_BY_OPCODE)

#undef CREATE_CREATORS
        default: {
            std::cerr << "Incorrect opcode!\n";
            return nullptr;
        }
    }
}

}
