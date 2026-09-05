// Golden file verification tests against tests/golden/*.expected for the Gravity VM.
#include <gtest/gtest.h>
#include "../src/vm/VM.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace luna;

namespace {

std::string read_file_from_candidates(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream buf;
                buf << file.rdbuf();
                return buf.str();
            }
        }
    }
    return "";
}

} // namespace

TEST(GoldenCorpus, MyScriptParity) {
    std::string script = read_file_from_candidates({
        "examples/myscript.luna",
        "../examples/myscript.luna",
        "../../examples/myscript.luna",
        "D:/Compiler Designing/CD/examples/myscript.luna"
    });
    ASSERT_FALSE(script.empty());

    std::string golden = read_file_from_candidates({
        "golden/myscript.expected",
        "tests/golden/myscript.expected",
        "../tests/golden/myscript.expected",
        "../../tests/golden/myscript.expected",
        "../../../tests/golden/myscript.expected",
        "D:/Compiler Designing/CD/tests/golden/myscript.expected"
    });
    ASSERT_FALSE(golden.empty());

    std::string vm_output;
    vm::VM vm;
    vm.set_output_handler([&vm_output](std::string_view text) {
        vm_output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);

    // Normalize \r\n to \n for cross-platform comparison
    std::string normalized_vm;
    for (char c : vm_output) {
        if (c != '\r') normalized_vm.push_back(c);
    }
    std::string normalized_golden;
    for (char c : golden) {
        if (c != '\r') normalized_golden.push_back(c);
    }

    EXPECT_EQ(normalized_vm, normalized_golden);
}

TEST(GoldenCorpus, WebsiteExampleExecution) {
    std::string script = read_file_from_candidates({
        "examples/website-example.luna",
        "../examples/website-example.luna",
        "../../examples/website-example.luna",
        "D:/Compiler Designing/CD/examples/website-example.luna"
    });
    ASSERT_FALSE(script.empty());

    std::string vm_output;
    vm::VM vm;
    vm.set_output_handler([&vm_output](std::string_view text) {
        vm_output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_FALSE(vm_output.empty());
    EXPECT_NE(vm_output.find("3628800"), std::string::npos);
}

TEST(GoldenCorpus, MandelbrotExecution) {
    std::string script = read_file_from_candidates({
        "examples/mandel.luna",
        "../examples/mandel.luna",
        "../../examples/mandel.luna",
        "D:/Compiler Designing/CD/examples/mandel.luna"
    });
    ASSERT_FALSE(script.empty());

    std::string vm_output;
    vm::VM vm;
    vm.set_output_handler([&vm_output](std::string_view text) {
        vm_output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_FALSE(vm_output.empty());
}

TEST(GoldenCorpus, DragonCurveExecution) {
    std::string script = read_file_from_candidates({
        "examples/dragon.luna",
        "../examples/dragon.luna",
        "../../examples/dragon.luna",
        "D:/Compiler Designing/CD/examples/dragon.luna"
    });
    ASSERT_FALSE(script.empty());

    std::string vm_output;
    vm::VM vm;
    vm.set_output_handler([&vm_output](std::string_view text) {
        vm_output.append(text);
    });

    auto res = vm.interpret(script);
    EXPECT_EQ(res, vm::InterpretResult::Ok);
    EXPECT_FALSE(vm_output.empty());
    EXPECT_NE(vm_output.find("line"), std::string::npos);
}
