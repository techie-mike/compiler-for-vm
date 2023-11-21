#include <gtest/gtest.h>
#include <ostream>
#include "graph.h"

#include "ir_constructor.h"
#include "tests/graph_comparator.h"
#include "optimizations/analysis/rpo.h"
#include "optimizations/analysis/domtree.h"
#include "optimizations/analysis/loop_analysis.h"

#include "optimizations/gcm.h"
#include "optimizations/analysis/linear_order.h"
#include "optimizations/analysis/liveness_analyzer.h"


namespace compiler {

void RegionInsideLoop(RegionInst *region, Loop *loop) {
    ASSERT_EQ(region->GetLoop(), loop);
    ASSERT_TRUE(loop->LoopContaine(region));
}

void CheckOrderPlacedInsts(Graph *graph, id_t index_region, std::vector<id_t> order) {
    auto region = graph->GetInstByIndex(index_region)->CastToRegion();
    auto real_inst = region->GetFirst();
    auto num = 0;

    for (auto index : order) {
        auto expected_inst = graph->GetInstByIndex(index);
        ASSERT_EQ(real_inst, expected_inst);

        real_inst = real_inst->GetNext();
        num++;
        if (real_inst == nullptr) {
            break;
        }
    }
    ASSERT_EQ(num, order.size());
}

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
    rpo.Run();

    std::vector<id_t> true_rpo = {0, 2, 3, 11, 4, 5, 9, 8, 6, 1};

    ASSERT_EQ(true_rpo.size(), rpo.GetVector().size());
    for (id_t i = 0; i < rpo.GetVector().size(); i++) {
        ASSERT_EQ(true_rpo[i], rpo.GetVector()[i]->GetId());
    }
}

/* SHOW ONLY REGION FLOW
 * 0.Start
 *    |
 *    v
 * 3.Region <--+
 *    |        |
 *    v        |
 * 5.Region    |
 *   |  |      |
 *   |  +------+
 *   v
 * 7.Region
 *    |
 *    v
 *  1.End
 */
TEST(AnalysisTest, LoopAnalysis1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Jump>(4).CtrlInput(3).JmpTo(5);

    ic.CreateInst<Opcode::Constant>(11).Imm(0);
    ic.CreateInst<Opcode::Region>(5);
    ic.CreateInst<Opcode::If>(6).CtrlInput(5).DataInputs(11).Branches(3, 7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Constant>(9).Imm(2);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(7).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);

    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    auto root_loop = ic.GetRegion(0)->GetLoop();
    auto loop1 = ic.GetRegion(3)->GetLoop();
    // In "root" loop
    RegionInsideLoop(ic.GetRegion(1), root_loop);
    RegionInsideLoop(ic.GetRegion(7), root_loop);

    // Headers of loops
    ASSERT_EQ(root_loop->GetHeader(), nullptr);
    ASSERT_EQ(loop1->GetHeader(), ic.GetRegion(3));

    // In loop 1, which inside of "root" loop
    RegionInsideLoop(ic.GetRegion(3), loop1);
    RegionInsideLoop(ic.GetRegion(5), loop1);

    // Outer loop of "loop 1" is "root" loop, inside loop of "root" loop is "loop 1" and unique
    ASSERT_EQ(loop1->GetOuterLoop(), root_loop);
    ASSERT_EQ(root_loop->GetInnerLoops().size(), 1);
    ASSERT_EQ(root_loop->GetInnerLoops()[0], loop1);

    // Check depth
    ASSERT_EQ(root_loop->GetDepth(), 0);
    ASSERT_EQ(loop1->GetDepth(), 1);
}

/* SHOW ONLY REGION FLOW
 *    0.Start
 *       |
 *       v
 *    3.Region <-+
 *       |       |
 *       v       |
 * +->5.Region   |
 * |    |  |     |
 * |    |  +-----+
 * |    |
 * |    v
 * |  7.Region
 * |    | |
 * +----+ |
 *        v
 *    13.Region
 *        |
 *        v
 *     1.End
 */
 // NOT UNDERSTAND this graph is irreducible or not?
TEST(AnalysisTest, DISABLED_LoopAnalysis2) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Jump>(4).CtrlInput(3).JmpTo(5);

    ic.CreateInst<Opcode::Constant>(11).Imm(0);
    ic.CreateInst<Opcode::Region>(5);
    ic.CreateInst<Opcode::If>(6).CtrlInput(5).DataInputs(11).Branches(3, 14);

    ic.CreateInst<Opcode::Region>(14);
    ic.CreateInst<Opcode::Jump>(15).CtrlInput(14).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::If>(12).CtrlInput(7).DataInputs(11).Branches(5, 13);

    ic.CreateInst<Opcode::Region>(13);
    ic.CreateInst<Opcode::Constant>(9).Imm(2);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(13).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);


    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    ic.GetFinalGraph()->Dump(std::cerr);
}

/* SHOW ONLY REGION FLOW
 *         0.Start
 *            |
 *            v
 *         3.Region<--------+
 *           |  |           |
 *     +-----+  +---+       |
 *     |            |       |
 *     v            v       |
 * 6.Region      9.Region   |
 *     |             |      |
 *     |             |      |
 *     |             |      |
 *     |             v      |
 *     |         11.Region  |
 *     |             |      |
 *     +-----+       +------+
 *           v
 *         1.End
 */
TEST(AnalysisTest, LoopAnalysisTestExample1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Constant>(5).Imm(0);
    ic.CreateInst<Opcode::If>(4).CtrlInput(3).DataInputs(5).Branches(6, 9);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::Return>(7).CtrlInput(6).DataInputs(5);
    ic.CreateInst<Opcode::Jump>(8).CtrlInput(7).JmpTo(1);

    ic.CreateInst<Opcode::Region>(9);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(9).JmpTo(11);

    ic.CreateInst<Opcode::Region>(11);
    ic.CreateInst<Opcode::Jump>(12).CtrlInput(11).JmpTo(3);
    ic.CreateInst<Opcode::End>(1);

    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    // In "root" loop
    auto root_loop = ic.GetRegion(0)->GetLoop();
    RegionInsideLoop(ic.GetRegion(1), root_loop);
    RegionInsideLoop(ic.GetRegion(6), root_loop);

    // In loop 1, which inside of "root" loop
    auto loop1 = ic.GetRegion(3)->GetLoop();
    RegionInsideLoop(ic.GetRegion(9), loop1);
    RegionInsideLoop(ic.GetRegion(11), loop1);

    // Headers of loops
    ASSERT_EQ(root_loop->GetHeader(), nullptr);
    ASSERT_EQ(loop1->GetHeader(), ic.GetInst(3));

    // Outer loop of "loop 1" is "root" loop, inside loop of "root" loop is "loop 1" and unique
    ASSERT_EQ(loop1->GetOuterLoop(), root_loop);
    ASSERT_EQ(root_loop->GetInnerLoops().size(), 1);
    ASSERT_EQ(root_loop->GetInnerLoops()[0], loop1);

    // Check depth
    ASSERT_EQ(root_loop->GetDepth(), 0);
    ASSERT_EQ(loop1->GetDepth(), 1);
}

/* SHOW ONLY REGION FLOW
 *           0.Start
 *              |
 *              v
 *           3.Region<------------+
 *             |                  |
 *    +--------+                  |
 *    |                           |
 *    |                           |
 *    v                           |
 * 6.Region  +->12.Region  +->14.Region
 *    | |    |     |  |    |
 *    | +----+     |  +----+
 *    |            |
 *    |            |
 *    +---+  +-----+
 *        v  v
 *      9.Region
 *          |
 *          |
 *          v
 *        1.End
 */
TEST(AnalysisTest, LoopAnalysisTestExample2) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(5).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Jump>(4).CtrlInput(3).JmpTo(6);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::If>(8).CtrlInput(6).DataInputs(5).Branches(9, 12);

    ic.CreateInst<Opcode::Region>(9);
    ic.CreateInst<Opcode::Return>(11).DataInputs(5).CtrlInput(9);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(11).JmpTo(1);

    ic.CreateInst<Opcode::Region>(12);
    ic.CreateInst<Opcode::If>(13).CtrlInput(12).DataInputs(5).Branches(9, 14);

    ic.CreateInst<Opcode::Region>(14);
    ic.CreateInst<Opcode::Jump>(15).CtrlInput(14).JmpTo(3);

    ic.CreateInst<Opcode::End>(1);

    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    auto root_loop = ic.GetRegion(0)->GetLoop();
    RegionInsideLoop(ic.GetRegion(1), root_loop);
    RegionInsideLoop(ic.GetRegion(9), root_loop);

    // In loop 1, which inside of "root" loop
    auto loop1 = ic.GetRegion(3)->GetLoop();
    RegionInsideLoop(ic.GetRegion(6), loop1);
    RegionInsideLoop(ic.GetRegion(12), loop1);
    RegionInsideLoop(ic.GetRegion(14), loop1);

    // Headers of loops
    ASSERT_EQ(root_loop->GetHeader(), nullptr);
    ASSERT_EQ(loop1->GetHeader(), ic.GetInst(3));

    // Outer loop of "loop 1" is "root" loop, inside loop of "root" loop is "loop 1" and unique
    ASSERT_EQ(loop1->GetOuterLoop(), root_loop);
    ASSERT_EQ(root_loop->GetInnerLoops().size(), 1);
    ASSERT_EQ(root_loop->GetInnerLoops()[0], loop1);

    // Check depth
    ASSERT_EQ(root_loop->GetDepth(), 0);
    ASSERT_EQ(loop1->GetDepth(), 1);
}

/* SHOW ONLY REGION FLOW
 *                  0.Start
 *                     |
 *                     v
 *                  3.Region<------------+
 *                     |                 |
 *                     v                 |
 *                  6.Region<---------+  |
 *                    | |             |  |
 *              +-----+ +------+      |  |
 *              v              v      |  |
 *          9.Region       12.Region  |  |
 *             | |             |      |  |
 *      +------+ +----+ +------+      |  |
 *      v             v v             |  |
 * 14.Region       17.Region          |  |
 *      |              |              |  |
 *      v              v              |  |
 *   1.End         19.Region----------+  |
 *                     |                 |
 *                     v                 |
 *                 21.Region-------------+
 */
TEST(AnalysisTest, LoopAnalysisTestExample3) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(5).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Jump>(4).CtrlInput(3).JmpTo(6);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::If>(8).CtrlInput(6).DataInputs(5).Branches(9, 12);

    ic.CreateInst<Opcode::Region>(9);
    ic.CreateInst<Opcode::If>(10).CtrlInput(9).DataInputs(5).Branches(14, 17);

    ic.CreateInst<Opcode::Region>(14);
    ic.CreateInst<Opcode::Return>(15).DataInputs(5).CtrlInput(14);
    ic.CreateInst<Opcode::Jump>(16).CtrlInput(15).JmpTo(1);

    ic.CreateInst<Opcode::Region>(12);
    ic.CreateInst<Opcode::Jump>(13).CtrlInput(12).JmpTo(17);

    ic.CreateInst<Opcode::Region>(17);
    ic.CreateInst<Opcode::Jump>(18).CtrlInput(17).JmpTo(19);


    ic.CreateInst<Opcode::Region>(19);
    ic.CreateInst<Opcode::If>(20).CtrlInput(19).DataInputs(5).Branches(21, 6);

    ic.CreateInst<Opcode::Region>(21);
    ic.CreateInst<Opcode::Jump>(22).CtrlInput(21).JmpTo(3);

    ic.CreateInst<Opcode::End>(1);

    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    auto root_loop = ic.GetRegion(0)->GetLoop();
    RegionInsideLoop(ic.GetRegion(1), root_loop);
    RegionInsideLoop(ic.GetRegion(14), root_loop);

    // In loop 1, which inside of "root" loop
    auto loop1 = ic.GetRegion(3)->GetLoop();
    RegionInsideLoop(ic.GetRegion(3), loop1);
    RegionInsideLoop(ic.GetRegion(21), loop1);

    // In loop 2, which inside of loop 1
    auto loop2 = ic.GetRegion(6)->GetLoop();
    RegionInsideLoop(ic.GetRegion(6), loop2);
    RegionInsideLoop(ic.GetRegion(12), loop2);
    RegionInsideLoop(ic.GetRegion(17), loop2);
    RegionInsideLoop(ic.GetRegion(19), loop2);

    // Headers of loops
    ASSERT_EQ(root_loop->GetHeader(), nullptr);
    ASSERT_EQ(loop1->GetHeader(), ic.GetInst(3));
    ASSERT_EQ(loop2->GetHeader(), ic.GetInst(6));

    // Outer loop of "loop 1" is "root" loop, inside loop of "root" loop is "loop 1" and unique
    // Outer loop of "loop 2" is "loop 1", inside of "loop 1" loop is "loop 2" and unique
    ASSERT_EQ(loop1->GetOuterLoop(), root_loop);
    ASSERT_EQ(root_loop->GetInnerLoops().size(), 1);
    ASSERT_EQ(root_loop->GetInnerLoops()[0], loop1);

    ASSERT_EQ(loop2->GetOuterLoop(), loop1);
    ASSERT_EQ(loop1->GetInnerLoops().size(), 1);
    ASSERT_EQ(loop1->GetInnerLoops()[0], loop2);

    // Check depth
    ASSERT_EQ(root_loop->GetDepth(), 0);
    ASSERT_EQ(loop1->GetDepth(), 1);
    ASSERT_EQ(loop2->GetDepth(), 2);
}

/*
 *                    +-------+
 *                    |0.Start|
 *                    +---+---+
 *                        |
 *                        v
 *                   +---------+
 *                   |    A    |
 *                   +---------+
 *                   |2.Region |
 *                   +----+----+
 *                        |
 *                        v
 *                   +---------+
 *                   |    B    |
 *                   +---------+
 *                   |4.Region |
 *                   +-+-----+-+
 *                     |     |
 *       +-------------+     +------------+
 *       v                                v
 *  +---------+      +---------+     +---------+
 *  |    C    |      |    E    |     |    F    |
 *  +---------+      +---------+<-+  +---------+
 *  |7.Region |      |11.Region|  |  |9.Region |
 *  +-----+---+      +--+------+  |  +--+--+---+
 *        |             |         |     |  |
 *        +-----+   +---+         +-----+  |
 *              v   v                      |
 *           +---------+      +---------+  |
 *           |    D    |      |    G    |  |
 *           +---------+<--+  +---------+<-+
 *           |13.Region|   |  |16.Region|
 *           +----+----+   |  +--+------+
 *                |        |     |
 *                v        +-----+
 *             +-----+
 *             |1.End|
 *             +-----+
 */
TEST(AnalysisTest, LoopAnalysisTestExample4) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(6).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(18).CtrlInput(0).JmpTo(2);

    // Block A
    ic.CreateInst<Opcode::Region>(2);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(2).JmpTo(4);

    // Block B
    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::If>(5).CtrlInput(4).DataInputs(6).Branches(7, 9);

    // Block C
    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Jump>(8).CtrlInput(7).JmpTo(13);

    // Block F
    ic.CreateInst<Opcode::Region>(9);
    ic.CreateInst<Opcode::If>(10).CtrlInput(9).DataInputs(6).Branches(11, 16);

    // Block E
    ic.CreateInst<Opcode::Region>(11);
    ic.CreateInst<Opcode::Jump>(12).CtrlInput(11).JmpTo(13);

    // Block D
    ic.CreateInst<Opcode::Region>(13);
    ic.CreateInst<Opcode::Return>(14).CtrlInput(13).DataInputs(6);
    ic.CreateInst<Opcode::Jump>(15).CtrlInput(14).JmpTo(1);

    // Block G
    ic.CreateInst<Opcode::Region>(16);
    ic.CreateInst<Opcode::Jump>(17).CtrlInput(16).JmpTo(13);

    ic.CreateInst<Opcode::End>(1);

    auto la = LoopAnalysis(ic.GetFinalGraph());
    la.Run();

    auto root_loop = ic.GetRegion(0)->GetLoop();
    RegionInsideLoop(ic.GetRegion(0), root_loop);
    RegionInsideLoop(ic.GetRegion(1), root_loop);
    RegionInsideLoop(ic.GetRegion(2), root_loop);
    RegionInsideLoop(ic.GetRegion(4), root_loop);
    RegionInsideLoop(ic.GetRegion(7), root_loop);
    RegionInsideLoop(ic.GetRegion(9), root_loop);
    RegionInsideLoop(ic.GetRegion(11), root_loop);
    RegionInsideLoop(ic.GetRegion(13), root_loop);
    RegionInsideLoop(ic.GetRegion(16), root_loop);

    // Header of loop
    ASSERT_EQ(root_loop->GetHeader(), nullptr);

    // Check outer loops
    ASSERT_EQ(root_loop->GetOuterLoop(), nullptr);
    ASSERT_EQ(root_loop->GetInnerLoops().size(), 0);

    // Check depth
    ASSERT_EQ(root_loop->GetDepth(), 0);
}

TEST(GcmTest, GcmTest1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Constant>(2).Imm(123);
    ic.CreateInst<Opcode::Add>(4).DataInputs(2, 2);
    ic.CreateInst<Opcode::Sub>(5).DataInputs(2, 2);
    ic.CreateInst<Opcode::Mul>(6).DataInputs(2, 2);
    ic.CreateInst<Opcode::Div>(7).DataInputs(2, 2);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();
    GCM(graph).Run();

    CheckOrderPlacedInsts(graph, 0, {2, 10});
    CheckOrderPlacedInsts(graph, 3, {4, 8, 9});
    CheckOrderPlacedInsts(graph, 1, {});
}

TEST(GcmTest, GcmTest2) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Constant>(8).Imm(2);
    ic.CreateInst<Opcode::Constant>(2).Imm(0);
    ic.CreateInst<Opcode::Phi>(9).CtrlInput(7).DataInputs(8, 2);

    ic.CreateInst<Opcode::Constant>(3).Imm(1);
    ic.CreateInst<Opcode::Compare>(4).DataInputs(2, 3).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::If>(5).CtrlInput(9).DataInputs(4).Branches(6, 7);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::Jump>(11).CtrlInput(6);
    ic.CreateInst<Opcode::End>(1).CtrlInput(11);

    auto graph = ic.GetFinalGraph();
    GCM(graph).Run();

    CheckOrderPlacedInsts(graph, 0, {3, 2, 8, 10});
    CheckOrderPlacedInsts(graph, 7, {9, 5, 4});
    CheckOrderPlacedInsts(graph, 6, {11});
    CheckOrderPlacedInsts(graph, 1, {});
}

TEST(LinearOrder, LinearOrder1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(5).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Jump>(4).CtrlInput(3).JmpTo(6);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::If>(8).CtrlInput(6).DataInputs(5).Branches(9, 12);

    ic.CreateInst<Opcode::Region>(9);
    ic.CreateInst<Opcode::If>(10).CtrlInput(9).DataInputs(5).Branches(14, 17);

    ic.CreateInst<Opcode::Region>(14);
    ic.CreateInst<Opcode::Return>(15).DataInputs(5).CtrlInput(14);
    ic.CreateInst<Opcode::Jump>(16).CtrlInput(15).JmpTo(1);

    ic.CreateInst<Opcode::Region>(12);
    ic.CreateInst<Opcode::Jump>(13).CtrlInput(12).JmpTo(17);

    ic.CreateInst<Opcode::Region>(17);
    ic.CreateInst<Opcode::Jump>(18).CtrlInput(17).JmpTo(19);


    ic.CreateInst<Opcode::Region>(19);
    ic.CreateInst<Opcode::If>(20).CtrlInput(19).DataInputs(5).Branches(21, 6);

    ic.CreateInst<Opcode::Region>(21);
    ic.CreateInst<Opcode::Jump>(22).CtrlInput(21).JmpTo(3);

    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();

    GCM(graph).Run();
    graph->Dump(std::cerr);

    auto lo = LinearOrder(graph);
    lo.Run();

    std::vector<id_t> true_linear_order = {0, 3, 6, 12, 9, 17, 19, 21, 14, 1};

    ASSERT_EQ(true_linear_order.size(), lo.GetVector().size());
    for (id_t i = 0; i < lo.GetVector().size(); i++) {
        ASSERT_EQ(lo.GetVector()[i]->GetId(), true_linear_order[i]);
    }
}

TEST(LivenessAnalyzerTest, Test1) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Constant>(2).Imm(123);
    ic.CreateInst<Opcode::Add>(4).DataInputs(2, 2);
    ic.CreateInst<Opcode::Sub>(5).DataInputs(2, 2);
    ic.CreateInst<Opcode::Mul>(6).DataInputs(2, 2);
    ic.CreateInst<Opcode::Div>(7).DataInputs(2, 2);
    ic.CreateInst<Opcode::Return>(8).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();

    GCM(graph).Run();
    auto la = LivenessAnalyzer(graph);
    la.Run();
    graph->Dump(std::cerr);
}

}
