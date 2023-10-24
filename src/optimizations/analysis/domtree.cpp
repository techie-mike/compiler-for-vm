#include "domtree.h"
#include "rpo.h"

namespace compiler {

void DomTreeSlow::DFSRegions(Inst *region, std::vector<Inst *> &found, Marker &marker) {
    if (marker.IsMarked(region)) {
        return;
    }
    marker.SetMarker(region);
    found.push_back(region);

    auto branch = SkipBodyOfRegion(region);
    for (auto way : branch->GetRawUsers()) {
        DFSRegions(way, found, marker);
    }
}

void DomTreeSlow::Run() {
    std::vector<Inst *> full_dfs;
    Marker marker(graph_);
    // TODO: Add "GetStartRegion", "GetEndRegion"
    DFSRegions(graph_->GetInstByIndex(0), full_dfs, marker);
    std::sort(full_dfs.begin(), full_dfs.end());

    std::vector<Inst *> part_dfs;
    std::vector<Inst *> diff;
    auto rpo = RpoRegions(graph_);
    rpo.Run();

    for (auto investigated : rpo.GetVector()) {
        diff.clear();
        part_dfs.clear();
        marker.Clear();
        marker.SetMarker(investigated);

        DFSRegions(graph_->GetInstByIndex(0), part_dfs, marker);
        std::sort(part_dfs.begin(), part_dfs.end());

        std::set_difference(full_dfs.begin(), full_dfs.end(), part_dfs.begin(), part_dfs.end(),
            std::inserter(diff, diff.begin()));

        for (auto it : diff) {
            if (it == investigated) {
                continue;
            }
            static_cast<RegionInst *>(investigated)->AddDominated(it);
        }
    }
}

}
