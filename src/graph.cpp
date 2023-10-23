#include "graph.h"
#include <iterator>
#include <ostream>
#include <iomanip>

namespace compiler {

void Graph::Dump(std::ostream &out)
{
    out << "Method: " << name_method_ << std::endl;
    out << "Instructions:" << std::endl;
    for (auto inst : all_inst_) {
        if (inst == nullptr) {
            continue;
        }
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
            ASSERT(inst->GetOpcode() == Opcode::OPCODE || inst->GetOpcode() == Opcode::NONE);   \
            inst->SetOpcode(Opcode::OPCODE);                                                    \
            return inst;                                                                        \
        }                                                                                       \

OPCODE_LIST(CREATER_BY_OPCODE)

#undef CREATER_BY_OPCODE
        default: {
            std::cerr << "Incorrect opcode!\n";
            return nullptr;
        }
    }
}

void Graph::DumpDomTree(std::ostream &out) {
    out << "Dominations in graph:\n";
    // TODO Create list of all CFG inst like region
    for (auto inst : all_inst_) {
        auto opc = inst->GetOpcode();
        if (opc != Opcode::Start && opc != Opcode::Region && opc != Opcode::End) {
            continue;
        }
        auto region = static_cast<RegionInst *>(inst);
        out << std::setw(4) << std::right << std::to_string(inst->GetId()) << ") ";
        if (region->GetDominator() == nullptr) {
            if (region != all_inst_[0]) {
                out << "NOT_SET";
            }
        } else {
            out << std::to_string(region->GetDominator()->GetId());
        }
        out << " -> ";
        bool first = true;
        for (auto dom : region->GetDominated()) {
            if (!first) {
                out << ", ";
            } else {
                first = false;
            }

            out << std::to_string(dom->GetId());
        }
        out << "\n";
    }
}


}
