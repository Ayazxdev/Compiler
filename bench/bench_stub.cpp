// bench/bench_stub.cpp — Google Benchmark smoke benchmark.
//
// Verifies that the Google Benchmark library is linked and the CMake target
// builds correctly.  These benchmarks are trivially cheap by design.

#include "luna/version.hpp"

#include <benchmark/benchmark.h>
#include <string_view>

// Smoke: verify version header is accessible from bench context

static void BM_VersionStringAccess(benchmark::State& state) {
    for (auto _ : state) {
        // Prevent the compiler from optimizing away the access.
        benchmark::DoNotOptimize(luna::VERSION_STRING.size());
    }
}
BENCHMARK(BM_VersionStringAccess);

// Smoke: string_view comparison

static void BM_StringViewCompare(benchmark::State& state) {
    constexpr std::string_view needle = "while";
    constexpr std::string_view haystack = "while";
    for (auto _ : state) {
        benchmark::DoNotOptimize(needle == haystack);
    }
}
BENCHMARK(BM_StringViewCompare);

// Smoke: character classification

static void BM_IsAlpha(benchmark::State& state) {
    const char* src = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::size_t i   = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(static_cast<bool>(std::isalpha(
            static_cast<unsigned char>(src[i++ % 36]))));
    }
}
BENCHMARK(BM_IsAlpha);
