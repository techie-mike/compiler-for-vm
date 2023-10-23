#include <gtest/gtest.h>
#include <ostream>
#include "graph.h"

#include "ir_constructor.h"
#include "tests/graph_comparator.h"
#include "optimizations/analysis/rpo.h"
#include "optimizations/analysis/domtree.h"


namespace compiler {

/*
 *             0.Start
 *                |
 *               \/
 *             2.Jump
 *                |
 *               \/
 *            3.Region
 *               |      11.Constant 0
 *               |            |
 *               | +----------+
 *               | |
 *              \/\/
 *     ++----- 4.If
 *     ||        |
 *     ||        \/
 *     |+--> 5.Region
 *     |         |
 *     |        \/
 *     +--->  6.Jump
 *               |
 *              \/
 *           7.Region   9.Constant 2
 *               |         |
 *               |  +------+
 *               |  |
 *              \/ \/
 *            8.Return
 *                |
 *               \/
 *            10.Jump
 *                |
 *               \/
 *            1.Start
 *
*/
TEST(AnalysisTest, RpoAnalysisRegionsInsts1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Constant>(11).Imm(0);
    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::If>(4).DataInputs(11).CtrlInput(3).Branches(5, 7);

    ic.CreateInst<Opcode::Region>(5);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Constant>(9).Imm(2);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(7).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();
    // ========================================================================
    auto rpo_regions = RpoRegions(graph);
    rpo_regions.Run();
    std::vector<id_t> true_rpo_regions = {0, 3, 5, 7, 1};

    ASSERT_EQ(true_rpo_regions.size(), rpo_regions.GetVector().size());
    for (id_t i = 0; i < rpo_regions.GetVector().size(); i++) {
        ASSERT_EQ(rpo_regions.GetVector()[i]->GetId(), true_rpo_regions[i]);
    }

    // ========================================================================
    auto rpo_insts = RpoInsts(graph);
    rpo_insts.Run();
    std::vector<id_t> true_rpo_insts = {0, 2, 3, 11, 4, 5, 6, 7, 9, 8, 10, 1};

    ASSERT_EQ(true_rpo_insts.size(), rpo_insts.GetVector().size());
    for (id_t i = 0; i < rpo_insts.GetVector().size(); i++) {
        ASSERT_EQ(rpo_insts.GetVector()[i]->GetId(), true_rpo_insts[i]);
    }

    // ========================================================================
    auto domtree = DomTreeSlow(graph);
    domtree.Run();

    std::ostringstream dump_out;
    std::string output =
    "Dominations in graph:\n"
    "   0)  -> 3, 5, 7, 1\n"
    "   1) 7 -> \n"
    "   3) 0 -> 5, 7, 1\n"
    "   5) 3 -> \n"
    "   7) 3 -> 1\n";
    graph->DumpDomTree(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(AnalysisTest, RpoAnalysisRegionsInsts2) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(9).Imm(2);
    ic.CreateInst<Opcode::Constant>(11).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::If>(4).DataInputs(11).CtrlInput(3).Branches(3, 5);

    ic.CreateInst<Opcode::Region>(5);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(5).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);

    auto rpo_regions = RpoRegions(ic.GetFinalGraph());
    rpo_regions.Run();
    std::vector<id_t> true_rpo_regions = {0, 3, 5, 1};

    auto rpo_insts = RpoInsts(ic.GetFinalGraph());
    rpo_insts.Run();
    std::vector<id_t> true_rpo_insts = {0, 2, 3, 11, 4, 5, 9, 8, 6, 1};

    ASSERT_EQ(true_rpo_regions.size(), rpo_regions.GetVector().size());
    for (id_t i = 0; i < rpo_regions.GetVector().size(); i++) {
        ASSERT_EQ(rpo_regions.GetVector()[i]->GetId(), true_rpo_regions[i]);
    }

    ASSERT_EQ(true_rpo_insts.size(), rpo_insts.GetVector().size());
    for (id_t i = 0; i < rpo_insts.GetVector().size(); i++) {
        ASSERT_EQ(rpo_insts.GetVector()[i]->GetId(), true_rpo_insts[i]);
    }
}

TEST(AnalysisTest, RpoAnalysisInsts) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(9).Imm(2);
    ic.CreateInst<Opcode::Constant>(11).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::If>(4).DataInputs(11).CtrlInput(3).Branches(3, 5);

    ic.CreateInst<Opcode::Region>(5);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(5).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto rpo = RpoInsts(ic.GetFinalGraph());
    // ic.GetFinalGraph()->Dump(std::cerr);
    rpo.Run();

    std::vector<id_t> true_rpo = {0, 2, 3, 11, 4, 5, 9, 8, 6, 1};

    ASSERT_EQ(true_rpo.size(), rpo.GetVector().size());
    for (id_t i = 0; i < rpo.GetVector().size(); i++) {
        ASSERT_EQ(true_rpo[i], rpo.GetVector()[i]->GetId());
    }
}

}
