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

    LifeNumber GetBegin() const {
        return begin_;
    }

    LifeNumber GetEnd() const {
        return end_;
    }


protected:
    LifeNumber begin_;
    LifeNumber end_;
};

class LiveInterval : public LiveRange {
public:
    LiveInterval():
        LiveRange() {};

    LiveInterval(LifeNumber begin, LifeNumber end):
        LiveRange(begin, end) {};

    void Append(LifeNumber begin, LifeNumber end) {
        if (is_first_) {
            begin_ = begin;
            end_ = end;
            is_first_ = false;
            return;
        }

        //                 OldBegin     OldEnd
        //                   ||-----------||
        //   ||-----------------||
        // begin                end
        // ==============RESULT==============
        //   ||---------------------------||
        //  NewBegin                    NewEnd
        if (begin < begin_ && end < end_) {
            begin_ = begin;
            return;
        }

        //          OldBegin     OldEnd
        //            ||-----------||
        //   ||--------------------------||
        // begin                         end
        // ==============RESULT==============
        //   ||--------------------------||
        //  NewBegin                    NewEnd
        if (begin < begin_ && end > end_) {
            begin_ = begin;
            end_ = end;
            return;
        }

        // OldBegin                     OldEnd
        //   ||--------------------------||
        //            ||-----------||
        //           begin         end
        // ==============RESULT==============
        //   ||--------------------------||
        //  NewBegin                    NewEnd
        if (begin > begin_ && end < end_) {
            return;
        }

        // OldBegin    OldEnd
        //   ||---------||
        //             ||-----------||
        //            begin         end
        // ==============RESULT==============
        //   ||---------------------||
        //  NewBegin               NewEnd
        if (begin > begin_ && end > end_) {
            return;
        }

        UNREACHABLE();
    }

    void TrimBegin(LifeNumber begin) {
        ASSERT(begin >= begin_);
        begin_ = begin;
    }

    void Dump() {
        std::cerr << "[" << begin_ << ", " << end_ << ")" << std::endl;
    }

    void SetLinearNumber(LinearNumber number) {
        my_linear_number_ = number;
    }

    LinearNumber GetLinearNumber() const {
        return my_linear_number_;
    }

private:
    bool is_first_ = true;
    LinearNumber my_linear_number_;
};

class RegionBlockLiveSet {
public:
    RegionBlockLiveSet(LifeNumber size):
        live_inst_(size, false),
        size_(size) {}

    RegionBlockLiveSet() = delete;

    void Set(LinearNumber index) {
        ASSERT(index < live_inst_.size());
        live_inst_[index] = true;
    }

    void Clear(LinearNumber index) {
        ASSERT(index < live_inst_.size());
        live_inst_[index] = false;
    }

    bool IsSet(LinearNumber index) {
        ASSERT(index < live_inst_.size());
        return live_inst_[index];
    }

    void Copy(RegionBlockLiveSet *live_set) {
        ASSERT(live_set->size_ == size_);
        for (LinearNumber i = 0; i < size_; i++) {
            live_inst_[i] = live_set->live_inst_[i];
        }
    }

    LinearNumber Size() {
        return size_;
    }

private:
    // std::vector<bool> possible have optimal realisation for bool
    std::vector<bool> live_inst_;
    LinearNumber size_ = 0;
};

class LivenessAnalyzer {
public:
    LivenessAnalyzer(Graph *graph):
        graph_(graph) {};

    ~LivenessAnalyzer() {
        for (auto liveset : region_block_livesets_) {
            delete liveset.second;
        }
    }

    void Run();
    void DumpLifeLinearData(std::ostream &out);
    std::vector<LiveInterval>& GetLiveIntervals() {
        return live_intervals_;
    }

private:
    void PrepareData();
    void BuildLifeNumbers();
    void FillLifeNumbersInRegionBlock(RegionInst *region, LifeNumber &life_number, LinearNumber &linear_number);
    void SetRegionLiveRanges(RegionInst *region, LiveRange &&range);
    void PrintLifeLinearData(Inst *inst, std::ostream &out);
    void BuildLifeIfJump(RegionInst *region, LinearNumber &linear_number, LifeNumber &life_number);

    void BuildIntervals();
    void CalcIniteialLiveSet(RegionInst *region);
    void UpdateLiveIntervalAllLiveSet(RegionBlockLiveSet* live_set, LifeNumber begin, LifeNumber end);
    void ReverseIterateRegionBlock(RegionInst *region);
    void ProcessHeaderRegion(RegionInst *region);
    void DumpIntervals();

    bool IsInstWithoutLife(Inst *inst) { // Oh my, instruction don't have life. It is sad... :(
        auto opc = inst->GetOpcode();
        return opc == Opcode::Jump;
    }

    bool HaveLifeInterval(Inst *inst) {
        auto opc = inst->GetOpcode();
        // Exception is instructions which have life_number and linear number, but doesn't have the live interval
        return !(opc == Opcode::If || opc == Opcode::Return);
    }


private:
    LinearNumber num_linear_inst_ = 0;
    Graph *graph_;
    std::vector<RegionInst *> linear_regions_;
    std::vector<LifeNumber> inst_life_numbers_;
    std::vector<LiveInterval> live_intervals_;

    // Unfortunately, I had to use a "map", since in Sea Of Nodes the region indices are not in order
    std::map<id_t, LiveRange> region_live_ranges_;
    std::map<id_t, RegionBlockLiveSet *> region_block_livesets_;
};

}
