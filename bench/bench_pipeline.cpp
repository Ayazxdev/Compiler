// End-to-End Google Benchmark Suite for the Compiler & Gravity Bytecode VM pipeline.
// Measures throughput for Lexer, Parser, TreeWalker oracle, and Bytecode VM.
#include <benchmark/benchmark.h>
#include "../src/lexer/Lexer.hpp"
#include "../src/parser/Parser.hpp"
#include "../src/memory/Arena.hpp"
#include "../src/resolver/Resolver.hpp"
#include "../src/compiler/Compiler.hpp"
#include "../src/vm/VM.hpp"
#include "../src/interpreter/TreeWalker.hpp"
#include <string_view>

using namespace luna;

constexpr std::string_view kFactorialScript = R"(
func factorial(n)
    if n <= 1 then
        ret 1
    else
        ret n * factorial(n - 1)
    end
end
x := factorial(10)
)";

constexpr std::string_view kLoopScript = R"(
sum := 0
for i := 1, 1000 do
    sum := sum + i
end
)";

// 1. Lexer throughput benchmark
static void BM_LexerThroughput(benchmark::State& state) {
    for (auto _ : state) {
        lexer::Lexer scanner(kFactorialScript);
        while (!scanner.at_end()) {
            auto tok = scanner.next();
            benchmark::DoNotOptimize(tok);
            if (tok.type == lexer::TokenType::Eof) break;
        }
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(kFactorialScript.size()));
}
BENCHMARK(BM_LexerThroughput);

// 2. Parser & AST Generation throughput benchmark
static void BM_ParserThroughput(benchmark::State& state) {
    for (auto _ : state) {
        memory::Arena arena;
        parser::Parser parser(kFactorialScript, arena);
        auto* prog = parser.parse();
        benchmark::DoNotOptimize(prog);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(kFactorialScript.size()));
}
BENCHMARK(BM_ParserThroughput);

// 3. Compiler bytecode generation benchmark
static void BM_CompilerThroughput(benchmark::State& state) {
    memory::Arena arena;
    parser::Parser parser(kFactorialScript, arena);
    auto* prog = parser.parse();

    for (auto _ : state) {
        compiler::Compiler comp;
        auto chunk = comp.compile(prog);
        benchmark::DoNotOptimize(chunk);
    }
}
BENCHMARK(BM_CompilerThroughput);

// 4. VM Factorial Execution benchmark
static void BM_VMExecutionFactorial(benchmark::State& state) {
    memory::Arena arena;
    parser::Parser parser(kFactorialScript, arena);
    auto* prog = parser.parse();
    compiler::Compiler comp;
    auto chunk = comp.compile(prog);

    for (auto _ : state) {
        vm::VM vm;
        auto res = vm.run(chunk);
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(BM_VMExecutionFactorial);

// 5. VM Loop Throughput benchmark
static void BM_VMExecutionLoop(benchmark::State& state) {
    memory::Arena arena;
    parser::Parser parser(kLoopScript, arena);
    auto* prog = parser.parse();
    compiler::Compiler comp;
    auto chunk = comp.compile(prog);

    for (auto _ : state) {
        vm::VM vm;
        auto res = vm.run(chunk);
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(BM_VMExecutionLoop);
