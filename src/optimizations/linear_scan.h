#pragma once

#include <string>
#include <list>
#include <vector>
#include "analysis/liveness_analyzer.h"

namespace compiler {

struct LocationData {
    LocationData(int32_t reg_index, std::string&& reg_name, bool it_is_reg):
        index(reg_index),
        name(reg_name),
        is_reg(it_is_reg) {};

    int32_t index;
    std::string name;
    bool is_reg;
};

struct Register: public LocationData {
    Register(int32_t reg_index, std::string&& reg_name):
        LocationData(reg_index, std::move(reg_name), true) {};

    Register(LocationData &location):
        LocationData(location) {
            ASSERT(location.is_reg);
        };
};

struct StackLocation: public LocationData {
    StackLocation(int32_t reg_index, std::string&& reg_name):
        LocationData(reg_index, std::move(reg_name), false) {};

    StackLocation(LocationData &location):
        LocationData(location) {
            ASSERT(!location.is_reg);
        };
};

struct RegMap {
    LiveInterval interval;
    LocationData location;
};

using RegMapLinkIndex = LinearNumber;

class LinearScanRegAlloc {
public:
    LinearScanRegAlloc(std::vector<LiveInterval>& intervals):
        live_intervals_(intervals) {

        auto num_linear_inst = intervals.size();
        regs_map_sorted_begin_.reserve(num_linear_inst);
        for (LinearNumber j = 0; j < num_linear_inst; j++) {
            regs_map_sorted_begin_.push_back({intervals[j], LocationData(-1, "NOT SET", false)});
        }
        std::sort(regs_map_sorted_begin_.begin(), regs_map_sorted_begin_.end(),
            [](RegMap &left, RegMap &right) {return left.interval.GetBegin() < right.interval.GetBegin();});

        for (uint32_t i = 0; i < num_regs_; i++) {
            free_regs_.push_back(Register(i, std::string("x") + std::to_string(i)));
        }
    }

    std::vector<RegMap>& GetRegsMap() {
        return regs_map_sorted_begin_;
    }

    void Run();

private:
    void ExpireOldIntervals(RegMap &map);
    void SpillAtInterval(RegMap &map, LinearNumber index);
    void AddNewStackLocation();
    void SortActionIndexes();
    void Dump();

private:
    std::list<Register> free_regs_;
    uint32_t num_regs_ = 3;
    uint32_t num_stack_locations_ = 0;
    std::list<StackLocation> free_stack_location_;
    std::vector<LiveInterval> live_intervals_;
    std::vector<RegMap> regs_map_sorted_begin_;
    std::list<RegMapLinkIndex> action_idx_sorted_end_;
};

}
