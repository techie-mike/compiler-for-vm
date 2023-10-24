#include "rpo.h"
#include "analysis.h"

namespace compiler {

RpoRegions::RpoRegions(Graph *graph):
    graph_ (graph) {}

void RpoRegions::Run() {
    auto start = graph_->GetInstByIndex(0);
    auto marker = Marker(graph_);
    DFSRegions(start, marker);
    std::reverse(rpo_regions_.begin(), rpo_regions_.end());
}

void RpoRegions::DFSRegions(Inst *init_region, [[maybe_unused]] Marker &marker) {
    if (marker.IsMarked(init_region)) {
        return;
    }

    marker.SetMarker(init_region);

    auto inst = SkipBodyOfRegion(init_region);
    if (inst->GetOpcode() == Opcode::Jump) {
        DFSRegions(inst->GetControlUser(), marker);
    }

    if (inst->GetOpcode() == Opcode::If) {
        DFSRegions(static_cast<IfInst *>(inst)->GetTrueBranch(), marker);
        DFSRegions(static_cast<IfInst *>(inst)->GetFalseBranch(), marker);
    }

    AddInstInVector(init_region);
}

std::vector<RegionInst *> &RpoRegions::GetVector() {
    return rpo_regions_;
}

void RpoRegions::AddInstInVector(Inst *inst) {
    ASSERT(inst->IsRegion());
    rpo_regions_.push_back(static_cast<RegionInst *>(inst));
}

RpoInsts::RpoInsts(Graph *graph):
    graph_ (graph) {}

void RpoInsts::Run() {
    auto end = graph_->GetInstByIndex(1);
    auto marker = Marker(graph_);
    DFSInsts(end, marker);
}

void RpoInsts::DFSInsts(Inst *inst, Marker &marker) {
    if (marker.IsMarked(inst)) {
        return;
    }
    marker.SetMarker(inst);

    for (id_t i = 0; i < inst->NumAllInputs(); i++) {
        DFSInsts(inst->GetRawInput(i), marker);
    }

    AddInstInVector(inst);
}

std::vector<Inst *> &RpoInsts::GetVector() {
    return rpo_insts_;
}

void RpoInsts::AddInstInVector(Inst *inst) {
    rpo_insts_.push_back(inst);
}

}
