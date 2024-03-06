#pragma once

namespace compiler {

class Inst;
class Graph;

bool ConstFoldingBinaryOp(Graph *graph, Inst *inst);

}
