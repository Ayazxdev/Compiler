// Comprehensive test suite covering the compiler toolchain & Gravity VM pipeline:
// Lexer, Parser, Arena, Resolver, Compiler, Value, and VM Execution.
#include <gtest/gtest.h>
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/memory/Arena.hpp"
#include "../src/resolver/Resolver.hpp"
#include "../src/compiler/Compiler.hpp"
#include "../src/compiler/Disassembler.hpp"
#include "../src/vm/Value.hpp"
#include "../src/vm/VM.hpp"
#include <sstream>

using namespace luna;

// Lexer tests
TEST(LexerPipeline, TokenizeKeywordsAndSymbols) {
    std::string_view src = "if 5 ~= 2 then x := 42.5 and true else null end";
    lexer::Lexer lexer(src);

    auto t1 = lexer.next();
    EXPECT_EQ(t1.type, lexer::TokenType::If);

    auto t2 = lexer.next();
    EXPECT_EQ(t2.type, lexer::TokenType::Integer);
    EXPECT_EQ(t2.lexeme, "5");

    auto t3 = lexer.next();
    EXPECT_EQ(t3.type, lexer::TokenType::Ne);

    auto t4 = lexer.next();
    EXPECT_EQ(t4.type, lexer::TokenType::Integer);
    EXPECT_EQ(t4.lexeme, "2");

    auto t5 = lexer.next();
    EXPECT_EQ(t5.type, lexer::TokenType::Then);

    auto t6 = lexer.next();
    EXPECT_EQ(t6.type, lexer::TokenType::Identifier);
    EXPECT_EQ(t6.lexeme, "x");

    auto t7 = lexer.next();
    EXPECT_EQ(t7.type, lexer::TokenType::Assign);

    auto t8 = lexer.next();
    EXPECT_EQ(t8.type, lexer::TokenType::Float);
    EXPECT_EQ(t8.lexeme, "42.5");

    auto t9 = lexer.next();
    EXPECT_EQ(t9.type, lexer::TokenType::And);

    auto t10 = lexer.next();
    EXPECT_EQ(t10.type, lexer::TokenType::True_);

    auto t11 = lexer.next();
    EXPECT_EQ(t11.type, lexer::TokenType::Else);

    auto t12 = lexer.next();
    EXPECT_EQ(t12.type, lexer::TokenType::Null);

    auto t13 = lexer.next();
    EXPECT_EQ(t13.type, lexer::TokenType::End);

    auto t14 = lexer.next();
    EXPECT_EQ(t14.type, lexer::TokenType::Eof);
}

TEST(LexerPipeline, CommentsAndStrings) {
    std::string_view src = "-- this is a comment\n\"hello world\" -- trailing comment";
    lexer::Lexer lexer(src);

    auto t1 = lexer.next();
    EXPECT_EQ(t1.type, lexer::TokenType::String);
    EXPECT_EQ(t1.lexeme, "\"hello world\"");

    auto t2 = lexer.next();
    EXPECT_EQ(t2.type, lexer::TokenType::Eof);
}

// Parser (Gravity) and Arena Memory tests
TEST(ParserPipeline, PrattExpressionPrecedence) {
    memory::Arena arena;
    std::string_view src = "x := 2 + 3 * 4 ^ 2";
    parser::Parser parser(src, arena);
    auto* prog = parser.parse();

    ASSERT_NE(prog, nullptr);
    ASSERT_EQ(prog->stmts.size(), 1u);

    auto** assign = std::get_if<ast::Assignment*>(&prog->stmts[0]);
    ASSERT_NE(assign, nullptr);
    ASSERT_NE(*assign, nullptr);

    // LHS is identifier 'x'
    auto** lhs = std::get_if<ast::Identifier*>(&(*assign)->left);
    ASSERT_NE(lhs, nullptr);
    EXPECT_EQ((*lhs)->name, "x");

    // RHS root must be '+' because '*' and '^' have higher precedence
    auto** plus = std::get_if<ast::BinOp*>(&(*assign)->right);
    ASSERT_NE(plus, nullptr);
    EXPECT_EQ((*plus)->op.type, lexer::TokenType::Plus);

    // Left of '+' is 2
    auto** two = std::get_if<ast::IntegerLit*>(&(*plus)->left);
    ASSERT_NE(two, nullptr);
    EXPECT_EQ((*two)->value, 2);

    // Right of '+' is (3 * (4 ^ 2))
    auto** mul = std::get_if<ast::BinOp*>(&(*plus)->right);
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ((*mul)->op.type, lexer::TokenType::Star);
}

TEST(ParserPipeline, FunctionDeclarationAndForLoop) {
    memory::Arena arena;
    std::string_view src = R"(
        func factorial(n)
            res := 1
            for i := 1, n do
                res := res * i
            end
            ret res
        end
    )";
    parser::Parser parser(src, arena);
    auto* prog = parser.parse();

    ASSERT_NE(prog, nullptr);
    ASSERT_EQ(prog->stmts.size(), 1u);

    auto** fn = std::get_if<ast::FuncDecl*>(&prog->stmts[0]);
    ASSERT_NE(fn, nullptr);
    ASSERT_NE(*fn, nullptr);
    EXPECT_EQ((*fn)->name, "factorial");
    ASSERT_EQ((*fn)->params.size(), 1u);
    EXPECT_EQ((*fn)->params[0]->name, "n");
}

// Value representation tests
TEST(ValuePipeline, ArithmeticAndStringConcat) {
    vm::Value num1(10.0);
    vm::Value num2(2.5);

    EXPECT_EQ((num1 + num2).as_number(), 12.5);
    EXPECT_EQ((num1 - num2).as_number(), 7.5);
    EXPECT_EQ((num1 * num2).as_number(), 25.0);
    EXPECT_EQ((num1 / num2).as_number(), 4.0);

    vm::Value str("Answer: ");
    vm::Value combined = str + num1;
    EXPECT_TRUE(combined.is_string());
    EXPECT_EQ(combined.as_string(), "Answer: 10.0");
}

// Resolver, Compiler, and VM Execution tests
TEST(VMPipeline, ExecuteArithmeticAndVariables) {
    std::string_view script = R"(
        x := 10
        y := 20
        z := x + y * 2
        println(z)
    )";

    std::string output;
    vm::VM vm;
    vm.set_output_handler([&output](std::string_view text) {
        output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_EQ(output, "50\n");
}

TEST(VMPipeline, ExecuteFactorialFunction) {
    std::string_view script = R"(
        func factorial(n)
            if n <= 1 then
                ret 1
            else
                ret n * factorial(n - 1)
            end
        end

        println(factorial(5))
    )";

    std::string output;
    vm::VM vm;
    vm.set_output_handler([&output](std::string_view text) {
        output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_EQ(output, "120\n");
}

TEST(VMPipeline, ExecuteWhileAndForLoops) {
    std::string_view script = R"(
        sum := 0
        for i := 1, 5 do
            sum := sum + i
        end
        println(sum)
    )";

    std::string output;
    vm::VM vm;
    vm.set_output_handler([&output](std::string_view text) {
        output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_EQ(output, "15\n");
}

TEST(VMPipeline, ExecuteMyScript) {
    std::string_view script = R"(
x := 0
x := x + 1

println("The value of the global x is " + x)

if 5 ~= 2 then
  y := x + 20
  println("I have access to the local y = " + y)
  println("I have also access to the global x = " + x)
else
  x := y
  println("Error, no local variable " + y)
end

i := 1
while i <= 10 do
  println("i = " + i)
  i := i + 1
end
    )";

    std::string output;
    vm::VM vm;
    vm.set_output_handler([&output](std::string_view text) {
        output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);

    std::string expected =
        "The value of the global x is 1.0\n"
        "I have access to the local y = 21.0\n"
        "I have also access to the global x = 1.0\n"
        "i = 1.0\n"
        "i = 2.0\n"
        "i = 3.0\n"
        "i = 4.0\n"
        "i = 5.0\n"
        "i = 6.0\n"
        "i = 7.0\n"
        "i = 8.0\n"
        "i = 9.0\n"
        "i = 10.0\n";

    EXPECT_EQ(output, expected);
}
