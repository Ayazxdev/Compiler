// Differential Testing Harness:
// Runs Luna programs through BOTH the reference TreeWalker and the optimized Gravity Bytecode VM,
// asserting exact output parity on every test case.
#include <gtest/gtest.h>
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/memory/Arena.hpp"
#include "../src/resolver/Resolver.hpp"
#include "../src/compiler/Compiler.hpp"
#include "../src/vm/VM.hpp"
#include "../src/interpreter/TreeWalker.hpp"
#include <string>
#include <string_view>

using namespace luna;

namespace {

void assert_differential_match(std::string_view source) {
    // 1. Run through reference TreeWalker
    std::string tw_output;
    {
        memory::Arena arena;
        parser::Parser parser(source, arena);
        ast::Stmts* prog = parser.parse();

        interpreter::TreeWalker walker;
        walker.set_output_handler([&tw_output](std::string_view text) {
            tw_output.append(text);
        });
        walker.interpret(prog);
    }

    // 2. Run through optimized Bytecode VM
    std::string vm_output;
    {
        vm::VM vm;
        vm.set_output_handler([&vm_output](std::string_view text) {
            vm_output.append(text);
        });
        auto res = vm.interpret(source);
        EXPECT_EQ(res, vm::InterpretResult::Ok);
    }

    // 3. Assert exact match between TreeWalker and VM outputs
    EXPECT_EQ(tw_output, vm_output);
}

} // namespace

TEST(DifferentialTesting, ArithmeticExpressions) {
    assert_differential_match(R"(
        x := 10 + 20 * 3
        y := (x - 5) / 5
        println("x = " + x)
        println("y = " + y)
    )");
}

TEST(DifferentialTesting, ConditionalsAndBlocks) {
    assert_differential_match(R"(
        a := 15
        b := 20
        if a < b then
            println("a is smaller: " + a)
        else
            println("b is smaller: " + b)
        end
    )");
}

TEST(DifferentialTesting, WhileLoopAccumulator) {
    assert_differential_match(R"(
        i := 1
        sum := 0
        while i <= 10 do
            sum := sum + i
            i := i + 1
        end
        println("sum = " + sum)
    )");
}

TEST(DifferentialTesting, ForLoopStep) {
    assert_differential_match(R"(
        for i := 1, 10 do
            if i % 2 == 0 then
                println("even: " + i)
            end
        end
    )");
}
