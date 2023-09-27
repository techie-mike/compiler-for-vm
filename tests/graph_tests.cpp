#include <gtest/gtest.h>
#include <ostream>
#include "inst.h"
#include "src/graph.h"

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
    "   0.  Constant 0x1\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

TEST(GraphTest, CreateAdd) {
    Graph graph;
    auto *cnst0 = graph.CreateConstantInst();
    cnst0->SetImm(0);
    auto *cnst1 = graph.CreateConstantInst();
    cnst1->SetImm(1);
    auto *inst = graph.CreateAddInst(Type::INT64, cnst0, cnst1);
    std::ostringstream dump_out;
    std::string output =
    "Method: \n"
    "Instructions:\n"
    "   0.  Constant 0x0\n"
    "   1.  Constant 0x1\n"
    "   2.       Add v0, v1\n";

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
    "   0.     Start \n"
    "   1.     Start v0\n";
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
    "   0.     Start \n"
    "   1.    Region v0\n"
    "   2.     Start v1\n";
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
    "   0.     Start \n"
    "   1.    Region v0, v3\n"
    "   2.  Constant 0x0\n"
    "   3.        If v1, v2\n"
    "   4.    Region v3\n"
    "   5.     Start v4\n";

    graph.Dump(dump_out);
    ASSERT_EQ(dump_out.str(), output);
}

}
