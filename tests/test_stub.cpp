// tests/test_stub.cpp — GoogleTest smoke test.
//
// Proves the test infrastructure (FetchContent + GoogleTest) is wired correctly
// and the luna:: namespace is accessible.
//
// These tests verify the build system and environment.

#include "luna/version.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <ranges>
#include <string_view>

// Version smoke tests

TEST(LunaVersion, MajorIsZero) {
    EXPECT_EQ(luna::VERSION_MAJOR, 0);
}

TEST(LunaVersion, MinorIsOne) {
    EXPECT_EQ(luna::VERSION_MINOR, 1);
}

TEST(LunaVersion, VersionStringMatchesComponents) {
    // "0.1.0"
    EXPECT_EQ(luna::VERSION_STRING, "0.1.0");
}

TEST(LunaVersion, BuildTagIsNotEmpty) {
    EXPECT_FALSE(luna::BUILD_TAG.empty());
}

// Basic C++20 feature checks
// Ensures the compiler actually supports C++20 features we rely on.

TEST(Cpp20, ConstevalStringView) {
    // constexpr string_view comparison (used extensively in the lexer)
    constexpr std::string_view kw = "while";
    static_assert(kw.size() == 5, "string_view::size must be constexpr");
    EXPECT_EQ(kw, "while");
}

TEST(Cpp20, RangesBasic) {
    // std::ranges is used in the lexer keyword lookup
    const int arr[] = {3, 1, 4, 1, 5, 9};
    auto it = std::ranges::find(arr, 5);
    ASSERT_NE(it, std::end(arr));
    EXPECT_EQ(*it, 5);
}

TEST(Cpp20, ConceptsCompile) {
    // Verify that concept syntax parses — no runtime assertion needed.
    // The body is just a concept-constrained lambda that is immediately discarded.
    auto identity = []<typename T>(T x) -> T { return x; };
    EXPECT_EQ(identity(42), 42);
    EXPECT_EQ(identity(3.14), 3.14);
}
