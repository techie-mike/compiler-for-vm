#include "linear_order.h"
#include "rpo.h"

namespace compiler {

void LinearOrder::Run() {
    // TODO: Fix temporary solution
    auto rpo = RpoRegions(graph_);
    rpo.Run();
    linear_order_ = rpo.GetVector();
}

std::vector<RegionInst *> &LinearOrder::GetVector() {
    return linear_order_;
}


}
