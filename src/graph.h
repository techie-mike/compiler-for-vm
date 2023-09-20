#pragma once

#include <vector>
#include <string>
#include "inst.h"

namespace compiler {

class Graph
{

private:
    std::vector<Inst *> all_inst_;
    std::string name_method;
};

}
