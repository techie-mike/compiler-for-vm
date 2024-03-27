#pragma once

#include <utility>
#include <map>
#include <optional>

#include "graph.h"

namespace compiler {

class Inlining {
public:
    Inlining(Graph *main_graph, std::vector<Graph *> additional_graphs);

    void Run();

private:
    void TryInlineCalls();
    void FillMapAdditionalGraphs(std::vector<Graph *> &additional_graphs);
    std::vector<Inst *> GetRPOVector();
    void FindAllCalls();
    bool CanBeInlined(const Graph *ext_graph);
    std::optional<Graph *> GetGraphFuncByCall(CallInst *call);
    void InlineFunc(CallInst *call, Graph *func_graph);
    // Return value: first non-start Region of copied external graph
    Inst *InsertExternalGraph(Graph *ext_graph);
    void UpdateParameters(CallInst *call);
    void UpdateReturn(Inst *call);
    void SingleReturn(Inst *call, RegionInst *end_region);
    void MultipleReturn(Inst *call, RegionInst *end, uint32_t num_returns);
    void UpdateCfgSubgraph(CallInst *call);
    void DeleteUnnecessaryInst();

private:
    Inst *last_new_cfg_ = nullptr;
    uint32_t max_inline_insts_       = 20;  // This small default value for testing
    uint32_t already_inlined_insts_  = 0;
    Graph *graph_;

    std::vector<Inst *> all_calls_;
    std::map<std::pair<std::string, uint32_t>, Graph *> map_;
    std::vector<Inst *> inlined_subgraph_;
};

}
