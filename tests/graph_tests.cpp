#include <gtest/gtest.h>
#include <ostream>
#include "inst.h"
#include "graph.h"

#include "ir_constructor.h"
#include "tests/graph_comparator.h"

namespace compiler {

TEST(GraphTest, DumpEmptyGraph) {
    Graph graph;
    graph.SetMethodName("Test method");
    std::ostringstream dump_out;
    std::string output =
    "Method: Test method\n"
    "Instructions:\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateConstant) {
    Graph graph;
    graph.SetMethodName("Only const");
    auto *cnst = graph.CreateConstantInst();
    cnst->SetImm(1);
    std::ostringstream dump_out;
    std::string output =
    "Method: Only const\n"
    "Instructions:\n"
    "   0.i64 Constant   0x1\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateAdd) {
    Graph graph;
    auto *cnst0 = graph.CreateConstantInst();
    cnst0->SetImm(0);
    auto *cnst1 = graph.CreateConstantInst();
    cnst1->SetImm(1);
    auto *inst = graph.CreateAddInst();
    inst->SetType(Type::INT64);
    inst->SetInput(0, cnst0);
    inst->SetInput(1, cnst1);
    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.i64 Constant   0x0 -> v2\n"
    "   1.i64 Constant   0x1 -> v2\n"
    "   2.i64 Add        v0, v1\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateStart) {
    Graph graph;
    auto *start = graph.CreateStartInst();
    auto *end = graph.CreateStartInst();
    end->SetInput(0, start);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v1\n"
    "   1.    Start      v0\n";
    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateRegions) {
    Graph graph;
    auto *start = graph.CreateStartInst();
    auto *reg1 = graph.CreateRegionInst();
    reg1->AddInput(start);
    auto *end = graph.CreateStartInst();
    end->SetInput(0, reg1);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start      \n"
    "   1.    Region     v0 -> v2\n"
    "   2.    Start      v1\n";
    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

/*
 *             Start
 *               |
 *               \/
 *             Region <-+
 *  Constant 0    |     |
 *       |        |     |
 *       \/      \/     |
 *       ------------   |
 *       |   If     |   |
 *       ------------   |
 *       |True|False|   |
 *       ------------   |
 *         |      |     |
 *        \/      +-->--+
 *      Region
 *        |
 *       \/
 *      Start
 */
TEST(GraphTest, CreateIfFullWorkGraph) {
    Graph graph;
    auto *start = graph.CreateStartInst();
    auto *reg_loop = graph.CreateRegionInst();
    reg_loop->AddInput(start);

    auto *cnst = graph.CreateConstantInst();
    auto *if_inst = graph.CreateIfInst();
    if_inst->SetInput(0, reg_loop);
    if_inst->SetInput(1, cnst);

    auto *region_end = graph.CreateRegionInst();
    if_inst->SetTrueBranch(region_end);
    if_inst->SetFalseBranch(reg_loop);

    auto *end = graph.CreateStartInst();
    end->SetInput(0, region_end);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start      \n"
    "   1.    Region     v0, v3 -> v3\n"
    "   2.i64 Constant   0x0 -> v3\n"
    "   3.    If         [T:->v4 F:->v1] v1, v2\n"
    "   4.    Region     v3 -> v5\n"
    "   5.    Start      v4\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, TestConstructorAndComparator) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(0).Imm(123);
    ic.CreateInst<Opcode::Add>(1).Inputs(0, 0);

    auto ic_after = IrConstructor();
    ic_after.CreateInst<Opcode::Constant>(0).Imm(123);
    ic_after.CreateInst<Opcode::Add>(1).Inputs(0, 0);

    GraphComparator(ic.GetGraph(), ic_after.GetGraph()).Compare();
}

TEST(GraphTest, TestBinaryOperations) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(0).Imm(123);
    ic.CreateInst<Opcode::Add>(1).Inputs(0, 0);
    ic.CreateInst<Opcode::Sub>(2).Inputs(0, 0);
    ic.CreateInst<Opcode::Mul>(3).Inputs(0, 0);
    ic.CreateInst<Opcode::Div>(4).Inputs(0, 0);

    auto graph = ic.GetGraph();

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.i64 Constant   0x7b -> v1, v2, v3, v4\n"
    "   1.    Add        v0, v0\n"
    "   2.    Sub        v0, v0\n"
    "   3.    Mul        v0, v0\n"
    "   4.    Div        v0, v0\n";
    graph->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

/*
 *                      Start
 * Const 1     Const 0    |
 *   |           |       \/
 *   |          \/     Region <-+
 *   +------> Compare     |     |
 *               |        |     |
 *              \/       \/     |
 *              ------------    |
 *              |   If     |    |
 *              ------------    |
 *              |True|False|    |
 *              ------------    |
 *                |      |      |
 *               \/      +-->---+
 *             Region
 *               |
 *              \/
 *             Start
 */

TEST(GraphTest, TestCompare) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Region>(1).Inputs(0);
    ic.CreateInst<Opcode::Constant>(2).Imm(0);
    ic.CreateInst<Opcode::Constant>(3).Imm(1);
    ic.CreateInst<Opcode::Compare>(4).Inputs(2, 3).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::If>(5).Inputs(1, 4).Branches(6, 1);
    ic.CreateInst<Opcode::Start>(7).Inputs(6);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v1\n"
    "   1.    Region     v0, v5 -> v5\n"
    "   2.i64 Constant   0x0 -> v4\n"
    "   3.i64 Constant   0x1 -> v4\n"
    "   4.b   Compare    EQ v2, v3 -> v5\n"
    "   5.    If         [T:->v6 F:->v1] v1, v4\n"
    "   6.    Region     v5 -> v7\n"
    "   7.    Start      v6\n";
    ic.GetGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

/*
 *                      Start
 * Const 1     Const 0    |
 *   |           |       \/
 *   |          \/     Region <-+
 *   +------> Compare     |     |
 *               |        |     |
 *              \/       \/     |
 *              ------------    |
 *              |   If     |    |
 *              ------------    |
 *              |True|False|    |
 *              ------------    |
 *                |      |      |
 *               \/      +-->---+
 *             Region
 *               |
 *              \/
 *             Start
 */

TEST(GraphTest, TestPhi) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Constant>(8).Imm(2);
    ic.CreateInst<Opcode::Region>(1).Inputs(0);
    ic.CreateInst<Opcode::Constant>(2).Imm(0);
    ic.CreateInst<Opcode::Phi>(9).Inputs(1, 8, 2);
    ic.CreateInst<Opcode::Constant>(3).Imm(1);
    ic.CreateInst<Opcode::Compare>(4).Inputs(2, 3).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::If>(5).Inputs(1, 4).Branches(6, 1);
    ic.CreateInst<Opcode::Start>(7).Inputs(6);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v1\n"
    "   1.    Region     v0, v5 -> v9, v5\n"
    "   2.i64 Constant   0x0 -> v9, v4\n"
    "   3.i64 Constant   0x1 -> v4\n"
    "   4.b   Compare    EQ v2, v3 -> v5\n"
    "   5.    If         [T:->v6 F:->v1] v1, v4\n"
    "   6.    Region     v5 -> v7\n"
    "   7.    Start      v6\n"
    "   8.i64 Constant   0x2 -> v9\n"
    "   9.    Phi        v1, v8(R0), v2(R5)\n";
    ic.GetGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, TestParameterReturn) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(1).Imm(2);
    ic.CreateInst<Opcode::Return>(2).Inputs(0, 1);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v2\n"
    "   1.    Parameter   -> v2\n"
    "   2.    Return     v0, v1\n";
    ic.GetGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

}
