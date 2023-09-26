#include "graph.h"
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

}
