#include "linear_scan.h"
#include "analysis/liveness_analyzer.h"

namespace compiler {


void LinearScanRegAlloc::Run() {
    for (LinearNumber index = 0; index < regs_map_sorted_begin_.size(); index++) {
        auto &map = regs_map_sorted_begin_[index];
        if (map.interval.GetBegin() == map.interval.GetEnd() && map.interval.GetBegin() == 0) {
            continue;
        }
        ExpireOldIntervals(map);

        if (free_regs_.empty()) {
            SpillAtInterval(map, index);
        } else {
            map.location = free_regs_.back();
            free_regs_.pop_back();
            action_idx_sorted_end_.push_back(index);
            SortActionIndexes();
        }
    }
}

void LinearScanRegAlloc::ExpireOldIntervals([[maybe_unused]]RegMap &map) {
    for (auto it = action_idx_sorted_end_.begin(); it != action_idx_sorted_end_.end(); it = action_idx_sorted_end_.begin()) {
        // Yes, it is look strage
        if (regs_map_sorted_begin_[*it].interval.GetEnd() > map.interval.GetBegin()) {
            return;
        }
        if (map.location.is_reg) {
            free_regs_.push_back(map.location);
        }
        action_idx_sorted_end_.pop_front();
    }
}

void LinearScanRegAlloc::SpillAtInterval(RegMap &map, LinearNumber index) {
    if (regs_map_sorted_begin_[action_idx_sorted_end_.back()].interval.GetEnd() > map.interval.GetEnd()) {
        map.location = regs_map_sorted_begin_[action_idx_sorted_end_.back()].location;
        if (free_stack_location_.empty()) {
            AddNewStackLocation();
        }
        regs_map_sorted_begin_[action_idx_sorted_end_.back()].location = free_stack_location_.back();
        free_stack_location_.pop_back();

        // Remove spilled from active
        action_idx_sorted_end_.pop_back();
        action_idx_sorted_end_.push_back(index);
        SortActionIndexes();
    } else {
        if (free_stack_location_.empty()) {
            AddNewStackLocation();
        }
        map.location = free_stack_location_.back();
        free_stack_location_.pop_back();
    }

}

void LinearScanRegAlloc::AddNewStackLocation() {
    num_stack_locations_++;
    free_stack_location_.push_back(StackLocation(num_stack_locations_, std::string("s") + std::to_string(num_stack_locations_)));
}

void LinearScanRegAlloc::SortActionIndexes() {
    action_idx_sorted_end_.sort([this](RegMapLinkIndex &left, RegMapLinkIndex &right)
        {return regs_map_sorted_begin_[left].interval.GetEnd() < regs_map_sorted_begin_[right].interval.GetEnd();});
}

void LinearScanRegAlloc::Dump() {
    for (auto map : regs_map_sorted_begin_) {
        std::cerr << "LinNum: " << map.interval.GetLinearNumber() << "(" << map.location.name << ")" << std::endl;
    }
}

}
