#pragma once

namespace compiler {

class Inst;
class RegionInst;

Inst *SkipBodyOfRegion(Inst *inst);
RegionInst *GetRegionByInputRegion(Inst *inst);


}
