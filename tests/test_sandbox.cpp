// Sandbox Hardening, Resource Limits, and Malformed Input Tests for Gravity VM.
#include <gtest/gtest.h>
#include "../src/vm/VM.hpp"
#include <string_view>
#include <vector>

using namespace luna;

TEST(SandboxHardening, EnforceInstructionQuotaOnInfiniteLoop) {
    std::string_view infinite_loop = R"(
        while true do
            x := 1
        end
    )";

    vm::ResourceLimits limits;
    limits.max_instructions = 1000; // Low limit to catch quickly
    vm::VM vm(limits);

    auto result = vm.interpret(infinite_loop);
    EXPECT_EQ(result, vm::InterpretResult::RuntimeError);
    EXPECT_GE(vm.instructions_executed(), 1000u);
}

TEST(SandboxHardening, EnforceMaxRecursionDepthQuota) {
    std::string_view infinite_recursion = R"(
        func recurse(n)
            ret recurse(n + 1)
        end
        recurse(0)
    )";

    vm::ResourceLimits limits;
    limits.max_frames = 50; // Quota of 50 frames
    vm::VM vm(limits);

    auto result = vm.interpret(infinite_recursion);
    EXPECT_EQ(result, vm::InterpretResult::RuntimeError);
}

TEST(SandboxHardening, MalformedSyntaxFuzzInputsDoNotCrash) {
    const std::vector<std::string_view> malformed_inputs = {
        "",
        "   \t\n  ",
        "--- comment only",
        "\"",
        "\"unterminated string...",
        "func (x) end",
        "if then else end",
        "while do end",
        "x := := 5",
        "for := 1, 10 do end",
        "10 + + * 5",
        "~= == ==",
        "??? ### @@@",
        "func f() ret f()", // missing end
        "a.b.c.d.e"
    };

    for (auto input : malformed_inputs) {
        vm::VM vm;
        // Must handle syntax/lexer/parser errors gracefully without UB, memory leaks, or crashes
        EXPECT_NO_THROW({
            auto res = vm.interpret(input);
            (void)res;
        });
    }
}

#include "../src/service/SandboxService.hpp"

TEST(SandboxService, ExecuteScriptWithBudget) {
    service::SandboxService svc;
    service::SandboxRequest req;
    req.code = "x := 10 + 20\nprintln(x)";
    req.max_instructions = 10000;

    auto resp = svc.execute(req);
    EXPECT_EQ(resp.status, service::ServiceStatus::Success);
    EXPECT_EQ(resp.output, "30\n");
    EXPECT_GT(resp.instructions_executed, 0u);
}
