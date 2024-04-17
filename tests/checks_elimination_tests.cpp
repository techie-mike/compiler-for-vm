#include <gtest/gtest.h>

#include "graph.h"
#include "ir_constructor.h"
#include "graph_comparator.h"
#include "optimizations/checks_elimination.h"

namespace compiler {

TEST(ChecksElimination, NullCheckSameRegion) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::NullCheck>(5).DataInputs(2).CtrlInput(4);
    ic.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic.CreateInst<Opcode::NullCheck>(7).DataInputs(2).CtrlInput(5);
    ic.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic2.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic2.CreateInst<Opcode::Region>(4);
    ic2.CreateInst<Opcode::NullCheck>(5).DataInputs(2).CtrlInput(4);
    ic2.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic2.CreateInst<Opcode::NullCheck>(7).DataInputs(2).CtrlInput(5);
    ic2.CreateInst<Opcode::Return>(8).DataInputs(5).CtrlInput(5);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic2.CreateInst<Opcode::End>(1);

    auto true_graph = ic2.GetFinalGraph();

    ChecksElimination(graph).Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ChecksElimination, NullCheckDifferentRegion) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::NullCheck>(5).DataInputs(2).CtrlInput(4);
    ic.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(5).JmpTo(11);

    ic.CreateInst<Opcode::Region>(11);
    ic.CreateInst<Opcode::NullCheck>(7).DataInputs(2).CtrlInput(11);
    ic.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();

    ChecksElimination(graph).Run();

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic2.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic2.CreateInst<Opcode::Region>(4);
    ic2.CreateInst<Opcode::NullCheck>(5).DataInputs(2).CtrlInput(4);
    ic2.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);
    ic2.CreateInst<Opcode::Jump>(10).CtrlInput(5).JmpTo(11);

    ic2.CreateInst<Opcode::Region>(11);
    ic2.CreateInst<Opcode::NullCheck>(7).DataInputs(2).CtrlInput(11);
    ic2.CreateInst<Opcode::Return>(8).DataInputs(5).CtrlInput(11);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic2.CreateInst<Opcode::End>(1);

    auto true_graph = ic2.GetFinalGraph();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ChecksElimination, BoundsCheckSameRegion) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic.CreateInst<Opcode::Parameter>(10).Imm(1);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 10).CtrlInput(4);
    ic.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 10).CtrlInput(5);
    ic.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic2.CreateInst<Opcode::Parameter>(10).Imm(1);
    ic2.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic2.CreateInst<Opcode::Region>(4);
    ic2.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 10).CtrlInput(4);
    ic2.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic2.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 10).CtrlInput(5);
    ic2.CreateInst<Opcode::Return>(8).DataInputs(5).CtrlInput(5);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic2.CreateInst<Opcode::End>(1);

    auto true_graph = ic2.GetFinalGraph();

    ChecksElimination(graph).Run();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ChecksElimination, BoundsCheckDifferentRegion) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic.CreateInst<Opcode::Parameter>(12).Imm(1);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 12).CtrlInput(4);
    ic.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(5).JmpTo(11);

    ic.CreateInst<Opcode::Region>(11);
    ic.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 12).CtrlInput(11);
    ic.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic.CreateInst<Opcode::End>(1);

    auto graph = ic.GetFinalGraph();

    ChecksElimination(graph).Run();

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic2.CreateInst<Opcode::Parameter>(12).Imm(1);
    ic2.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic2.CreateInst<Opcode::Region>(4);
    ic2.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 12).CtrlInput(4);
    ic2.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);
    ic2.CreateInst<Opcode::Jump>(10).CtrlInput(5).JmpTo(11);

    ic2.CreateInst<Opcode::Region>(11);
    ic2.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 12).CtrlInput(11);
    ic2.CreateInst<Opcode::Return>(8).DataInputs(5).CtrlInput(11);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic2.CreateInst<Opcode::End>(1);

    auto true_graph = ic2.GetFinalGraph();
    GraphComparator(true_graph, graph).Compare();
}

TEST(ChecksElimination, BoundsCheckSameRegionNotApplied) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic.CreateInst<Opcode::Parameter>(10).Imm(1);
    ic.CreateInst<Opcode::Constant>(11).Imm(22);
    ic.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic.CreateInst<Opcode::Region>(4);
    ic.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 10).CtrlInput(4);
    ic.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 11).CtrlInput(5);
    ic.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic.CreateInst<Opcode::End>(1);
    auto graph = ic.GetFinalGraph();

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(2).Imm(0);
    ic2.CreateInst<Opcode::Parameter>(10).Imm(1);
    ic2.CreateInst<Opcode::Constant>(11).Imm(22);
    ic2.CreateInst<Opcode::Jump>(3).CtrlInput(0).JmpTo(4);

    ic2.CreateInst<Opcode::Region>(4);
    ic2.CreateInst<Opcode::BoundsCheck>(5).DataInputs(2, 10).CtrlInput(4);
    ic2.CreateInst<Opcode::Compare>(6).DataInputs(5, 5).CC(ConditionCode::EQ);

    ic2.CreateInst<Opcode::BoundsCheck>(7).DataInputs(2, 11).CtrlInput(5);
    ic2.CreateInst<Opcode::Return>(8).DataInputs(7).CtrlInput(7);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(1);
    ic2.CreateInst<Opcode::End>(1);

    auto true_graph = ic2.GetFinalGraph();

    ChecksElimination(graph).Run();
    GraphComparator(true_graph, graph).Compare();
}

}