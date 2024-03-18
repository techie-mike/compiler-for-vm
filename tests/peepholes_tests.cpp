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
#include "optimizations/linear_scan.h"
#include "optimizations/peepholes.h"

namespace compiler {

TEST(PeepholesTest, SubZero) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic.CreateInst<Opcode::Constant>(8).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Sub>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(0);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Sub>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(7);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();

    GraphComparator(true_graph, graph).Compare();
}

TEST(PeepholesTest, ShrAfterShr) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic.CreateInst<Opcode::Constant>(8).Imm(3);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Shr>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Shl>(9).DataInputs(4, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(9);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(3);
    ic_true.CreateInst<Opcode::Constant>(10).Imm(0x7ffffffffffffff8);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Shr>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Shl>(9).DataInputs(4, 8);
    ic_true.CreateInst<Opcode::And>(11).DataInputs(7, 10);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(11);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(PeepholesTest, SubSub) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::End>(1);
    ic.CreateInst<Opcode::Parameter>(2);
    ic.CreateInst<Opcode::Constant>(3).Imm(10);
    ic.CreateInst<Opcode::Constant>(4).Imm(15);
    ic.CreateInst<Opcode::Sub>(5).DataInputs(2, 3);
    ic.CreateInst<Opcode::Sub>(6).DataInputs(5, 4);
    ic.CreateInst<Opcode::Return>(7).CtrlInput(0).DataInputs(6);
    ic.CreateInst<Opcode::Jump>(8).CtrlInput(7).JmpTo(1);

    auto graph = ic.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Parameter>(2);
    ic_true.CreateInst<Opcode::Constant>(3).Imm(10);
    ic_true.CreateInst<Opcode::Constant>(4).Imm(15);
    ic_true.CreateInst<Opcode::Constant>(9).Imm(25);

    ic_true.CreateInst<Opcode::Sub>(5).DataInputs(2, 3);
    ic_true.CreateInst<Opcode::Sub>(6).DataInputs(2, 9);
    ic_true.CreateInst<Opcode::Return>(7).CtrlInput(0).DataInputs(6);
    ic_true.CreateInst<Opcode::Jump>(8).CtrlInput(7).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();

    GraphComparator(true_graph, graph).Compare();
}



TEST(ConstFoldingTest, Sub) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(7).Imm(22);
    ic.CreateInst<Opcode::Constant>(8).Imm(3);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Sub>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Constant>(7).Imm(22);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(3);
    ic_true.CreateInst<Opcode::Constant>(9).Imm(19);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Sub>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(9);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ConstFoldingTest, Shl) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(7).Imm(25);
    ic.CreateInst<Opcode::Constant>(8).Imm(2);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Shl>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Constant>(7).Imm(25);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(2);
    ic_true.CreateInst<Opcode::Constant>(9).Imm(100);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Shl>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(9);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ConstFoldingTest, Or) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Constant>(7).Imm(0xf00);
    ic.CreateInst<Opcode::Constant>(8).Imm(0x00f);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Or>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Constant>(7).Imm(0xf00);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(0x00f);
    ic_true.CreateInst<Opcode::Constant>(9).Imm(0xf0f);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Or>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(9);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(PeepholesTest, OrZero) {
    // Before
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic.CreateInst<Opcode::Constant>(8).Imm(0);

    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic.CreateInst<Opcode::Region>(3);
    ic.CreateInst<Opcode::Or>(4).DataInputs(7, 8);
    ic.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(4);
    ic.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    // After
    auto ic_true = IrConstructor();
    ic_true.CreateInst<Opcode::Parameter>(7).Imm(0);
    ic_true.CreateInst<Opcode::Constant>(8).Imm(0);

    ic_true.CreateInst<Opcode::Start>(0);
    ic_true.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(3);

    ic_true.CreateInst<Opcode::Region>(3);
    ic_true.CreateInst<Opcode::Or>(4).DataInputs(7, 8);
    ic_true.CreateInst<Opcode::Return>(5).CtrlInput(3).DataInputs(7);
    ic_true.CreateInst<Opcode::Jump>(6).CtrlInput(5).JmpTo(1);

    ic_true.CreateInst<Opcode::End>(1);

    auto true_graph = ic_true.GetFinalGraph();
    auto ph = Peepholes(graph);
    ph.Run();

    GraphComparator(true_graph, graph).Compare();
}


}
