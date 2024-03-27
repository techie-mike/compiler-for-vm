#include <gtest/gtest.h>
#include <ostream>
#include "graph.h"

#include "ir_constructor.h"
#include "tests/graph_comparator.h"
#include "optimizations/analysis/rpo.h"
#include "optimizations/analysis/domtree.h"
#include "optimizations/analysis/loop_analysis.h"

#include "optimizations/inlining.h"

namespace compiler {

TEST(InliningTest, SimpleTestJump) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Call>(8).NameFunc("Foo").CtrlInput(7);
    ic.CreateInst<Opcode::Return>(11).CtrlInput(8).DataInputs(8);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(11).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto main_graph = ic.GetFinalGraph();
    main_graph->SetMethodName("main");

    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Constant>(3).Imm(0);
    ic2.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(5);

    ic2.CreateInst<Opcode::Region>(5);
    ic2.CreateInst<Opcode::Return>(4).CtrlInput(5).DataInputs(3);
    ic2.CreateInst<Opcode::Jump>(6).CtrlInput(4).JmpTo(1);

    ic2.CreateInst<Opcode::End>(1);

    ic2.FinalizeRegions();
    auto ext_graph = ic2.GetFinalGraph();
    ext_graph->SetMethodName("Foo");

    auto inl = Inlining(main_graph, {ext_graph});
    inl.Run();
    main_graph->Dump(std::cerr);
}

TEST(InliningTest, SimpleTestIf) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Constant>(5).Imm(26);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Call>(8).NameFunc("Foo").CtrlInput(7).DataInputs(5);
    ic.CreateInst<Opcode::Return>(11).CtrlInput(8).DataInputs(8);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(11).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto main_graph = ic.GetFinalGraph();
    main_graph->SetMethodName("main");

    // External method
    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(3).Imm(0);
    ic2.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(5);

    ic2.CreateInst<Opcode::Region>(5);
    ic2.CreateInst<Opcode::If>(7).CtrlInput(5).DataInputs(3).Branches(8, 10);

    ic2.CreateInst<Opcode::Region>(8);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(10);

    ic2.CreateInst<Opcode::Region>(10);
    ic2.CreateInst<Opcode::Return>(4).CtrlInput(10).DataInputs(3);
    ic2.CreateInst<Opcode::Jump>(6).CtrlInput(4).JmpTo(1);

    ic2.CreateInst<Opcode::End>(1);

    ic2.FinalizeRegions();
    auto ext_graph = ic2.GetFinalGraph();
    ext_graph->SetMethodName("Foo");
    ext_graph->SetNumParams(1);

    auto inl = Inlining(main_graph, {ext_graph});
    inl.Run();
    main_graph->Dump(std::cerr);
}

TEST(InliningTest, WillNotApplied) {
    auto ic = IrConstructor();
    ic.CreateInst<Opcode::Start>(0);
    ic.CreateInst<Opcode::Constant>(5).Imm(26);
    ic.CreateInst<Opcode::Jump>(10).CtrlInput(0).JmpTo(7);

    ic.CreateInst<Opcode::Region>(7);
    ic.CreateInst<Opcode::Call>(8).NameFunc("FooBar").CtrlInput(7).DataInputs(5);
    ic.CreateInst<Opcode::Return>(11).CtrlInput(8).DataInputs(8);
    ic.CreateInst<Opcode::Jump>(9).CtrlInput(11).JmpTo(1);

    ic.CreateInst<Opcode::End>(1);
    auto main_graph = ic.GetFinalGraph();
    main_graph->SetMethodName("main");

    // External method
    auto ic2 = IrConstructor();
    ic2.CreateInst<Opcode::Start>(0);
    ic2.CreateInst<Opcode::Parameter>(3).Imm(0);
    ic2.CreateInst<Opcode::Jump>(2).CtrlInput(0).JmpTo(5);

    ic2.CreateInst<Opcode::Region>(5);
    ic2.CreateInst<Opcode::If>(7).CtrlInput(5).DataInputs(3).Branches(8, 10);

    ic2.CreateInst<Opcode::Region>(8);
    ic2.CreateInst<Opcode::Jump>(9).CtrlInput(8).JmpTo(10);

    ic2.CreateInst<Opcode::Region>(10);
    ic2.CreateInst<Opcode::Return>(4).CtrlInput(10).DataInputs(3);
    ic2.CreateInst<Opcode::Jump>(6).CtrlInput(4).JmpTo(1);

    ic2.CreateInst<Opcode::End>(1);

    ic2.FinalizeRegions();
    auto ext_graph = ic2.GetFinalGraph();
    ext_graph->SetMethodName("Foo");
    ext_graph->SetNumParams(1);

    auto inl = Inlining(main_graph, {ext_graph});
    inl.Run();
    main_graph->Dump(std::cerr);
}


}
