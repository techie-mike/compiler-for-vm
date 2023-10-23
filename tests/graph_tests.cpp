#include <gtest/gtest.h>

#include "graph.h"
#include "ir_constructor.h"
#include "graph_comparator.h"

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
    inst->SetDataInput(0, cnst0);
    inst->SetDataInput(1, cnst1);
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
    auto *end = graph.CreateEndInst();
    end->SetControlInput(start);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v1\n"
    "   1.    End        v0\n";
    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateRegions) {
    Graph graph;
    auto *start = graph.CreateStartInst();
    auto *reg1 = graph.CreateRegionInst();
    reg1->AddInput(start);
    auto *end = graph.CreateEndInst();
    end->SetControlInput(reg1);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start      \n"
    "   1.    Region     v0 -> v2\n"
    "   2.    End        v1\n";
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
    reg_loop->SetRegionInput(0, start);
    // reg_loop->SetDataInput(0, start);

    auto *cnst = graph.CreateConstantInst();
    auto *if_inst = graph.CreateIfInst();
    if_inst->SetControlInput(reg_loop);
    if_inst->SetDataInput(0, cnst);

    auto *region_end = graph.CreateRegionInst();
    if_inst->SetTrueBranch(region_end);
    if_inst->SetFalseBranch(reg_loop);

    auto *jmp = graph.CreateJumpInst();
    jmp->SetControlInput(region_end);
    auto *end = graph.CreateEndInst();
    end->SetControlInput(jmp);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start      \n"
    "   1.    Region     v0, v3 -> v3\n"
    "   2.i64 Constant   0x0 -> v3\n"
    "   3.    If         v1, v2 -> T:v4, F:v1\n"
    "   4.    Region     v3 -> v5\n"
    "   5.    Jump       v4 -> v6\n"
    "   6.    End        v5\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, TestConstructorAndComparator) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(0).Imm(123);
    ic.CreateInst<Opcode::Add>(1).DataInputs(0, 0);

    auto ic_after = IrConstructor();
    ic_after.CreateInst<Opcode::Constant>(0).Imm(123);
    ic_after.CreateInst<Opcode::Add>(1).DataInputs(0, 0);

    GraphComparator(ic.GetFinalGraph(), ic_after.GetFinalGraph()).Compare();
}

TEST(GraphTest, TestBinaryOperations) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Region>(3).CtrlInput(0);
    ic.CreateInst<Opcode::Constant>(2).Imm(123);
    ic.CreateInst<Opcode::Add>(4).DataInputs(2, 2);
    ic.CreateInst<Opcode::Sub>(5).DataInputs(2, 2);
    ic.CreateInst<Opcode::Mul>(6).DataInputs(2, 2);
    ic.CreateInst<Opcode::Div>(7).DataInputs(2, 2);
    ic.CreateInst<Opcode::Jump>(8).CtrlInput(3);
    ic.CreateInst<Opcode::End>(1).CtrlInput(8);

    auto graph = ic.GetFinalGraph();

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v3\n"
    "   1.    End        v8\n"
    "   2.i64 Constant   0x7b -> v4, v5, v6, v7\n"
    "   3.    Region     v0 -> v8\n"
    "   4.    Add        v2, v2\n"
    "   5.    Sub        v2, v2\n"
    "   6.    Mul        v2, v2\n"
    "   7.    Div        v2, v2\n"
    "   8.    Jump       v3 -> v1\n";

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
 *              Jmp
 *               |
 *              \/
 *             End
 */

TEST(GraphTest, TestCompare) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(8).CtrlInput(0).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Constant>(2).Imm(0);
    ic.CreateInst<Opcode::Constant>(3).Imm(1);
    ic.CreateInst<Opcode::Compare>(4).DataInputs(2, 3).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::If>(5).CtrlInput(7).DataInputs(4).Branches(6, 7);

    ic.CreateInst<Opcode::Region>(6);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(6);
    ic.CreateInst<Opcode::End>(1).CtrlInput(9);

    ic.FinalizeRegions();

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v8\n"
    "   1.    End        v9\n"
    "   2.i64 Constant   0x0 -> v4\n"
    "   3.i64 Constant   0x1 -> v4\n"
    "   4.b   Compare    EQ v2, v3 -> v5\n"
    "   5.    If         v7, v4 -> T:v6, F:v7\n"
    "   6.    Region     v5 -> v9\n"
    "   7.    Region     v8, v5 -> v5\n"
    "   8.    Jump       v0 -> v7\n"
    "   9.    Jump       v6 -> v1\n";
    ic.GetFinalGraph()->Dump(dump_out);
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
 *               Jmp
 *                |
 *               \/
 *               End
 */

TEST(GraphTest, TestPhi) {
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

    ic.FinalizeRegions();

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v10\n"
    "   1.    End        v11\n"
    "   2.i64 Constant   0x0 -> v9, v4\n"
    "   3.i64 Constant   0x1 -> v4\n"
    "   4.b   Compare    EQ v2, v3 -> v5\n"
    "   5.    If         v9, v4 -> T:v6, F:v7\n"
    "   6.    Region     v5 -> v11\n"
    "   7.    Region     v10, v5 -> v9\n"
    "   8.i64 Constant   0x2 -> v9\n"
    "   9.    Phi        v7, v8(R10), v2(R5) -> v5\n"
    "  10.    Jump       v0 -> v7\n"
    "  11.    Jump       v6 -> v1\n";

    ic.GetFinalGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, TestParameterReturn) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Region>(2).CtrlInput(0);
    ic.CreateInst<Opcode::Parameter>(3).Imm(2);
    ic.CreateInst<Opcode::Return>(4).CtrlInput(2).DataInputs(3);
    ic.CreateInst<Opcode::End>(1).CtrlInput(4);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v2\n"
    "   1.    End        v4\n"
    "   2.    Region     v0 -> v4\n"
    "   3.    Parameter   -> v4\n"
    "   4.    Return     v2, v3 -> v1\n";
    ic.GetFinalGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, TestCall) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Region>(2).CtrlInput(0);
    ic.CreateInst<Opcode::Constant>(3).Imm(4);
    ic.CreateInst<Opcode::Call>(4).CtrlInput(2).DataInputs(3);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(4).DataInputs(4);
    ic.CreateInst<Opcode::End>(1).CtrlInput(5);

    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.    Start       -> v2\n"
    "   1.    End        v5\n"
    "   2.    Region     v0 -> v4\n"
    "   3.i64 Constant   0x4 -> v4\n"
    "   4.    Call       v2, v3 -> v5, v5\n"
    "   5.    Return     v4, v4 -> v1\n";
    ic.GetFinalGraph()->Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

}
