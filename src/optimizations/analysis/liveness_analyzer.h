#pragma once

#include <map>
#include "inst.h"


namespace compiler {

class Graph;

class LiveRange {
public:
    LiveRange(LifeNumber begin, LifeNumber end):
        begin_(begin), end_(end) {}

    LiveRange():
        begin_(0), end_(0) {};

private:
    LifeNumber begin_;
    LifeNumber end_;
};

class LivenessAnalyzer {
public:
    LivenessAnalyzer(Graph *graph):
        graph_(graph) {};

    void Run();

    void DumpLifeLinearData(std::ostream &out);

private:
    void PrepareData();
    void BuildLifeNumbers();
    void SetRegionLiveRanges(RegionInst *region, LiveRange &&range);
    void PrintLifeLinearData(Inst *inst, std::ostream &out);
    void BuildLifeIfJump(RegionInst *region, LinearNumber &linear_number, LifeNumber &life_number);

private:
    Graph *graph_;
    std::vector<RegionInst *> linear_regions_;
    std::vector<LifeNumber> inst_life_numbers_;
    // Unfortunately, I had to use a "map", since in Sea Of Nodes the region indices are not in order
    std::map<id_t, LiveRange> region_live_ranges_;
};

}
