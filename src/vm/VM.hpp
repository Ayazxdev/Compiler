#pragma once
// Gravity Bytecode Virtual Machine Sandbox Execution Engine & Resource Limits.
#include "Value.hpp"
#include "../compiler/Chunk.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace luna::vm {

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

struct ResourceLimits {
    size_t max_instructions{100'000'000}; // Step limit (sandbox protection)
    size_t max_frames{1024};              // Call-stack depth quota
    size_t max_stack{65536};              // Evaluation stack ceiling
};

struct CallFrame {
    Value::FunctionPtr function;
    size_t             ip{0};
    size_t             slots{0}; // base index in stack
};

class VM {
public:
    VM();
    explicit VM(ResourceLimits limits);
    ~VM() = default;

    // High-level execution entry point: Lex -> Parse -> Resolve -> Compile -> Run
    InterpretResult interpret(std::string_view source);

    // Direct bytecode chunk execution
    InterpretResult run(std::shared_ptr<compiler::Chunk> chunk);

    // Resource limits / Sandbox configuration
    void set_resource_limits(ResourceLimits limits) noexcept {
        limits_ = limits;
    }
    [[nodiscard]] const ResourceLimits& resource_limits() const noexcept {
        return limits_;
    }
    [[nodiscard]] size_t instructions_executed() const noexcept {
        return instructions_executed_;
    }

    // Optional I/O interception (for tests/sandboxing)
    void set_output_handler(std::function<void(std::string_view)> handler) {
        output_handler_ = std::move(handler);
    }

    // Access globals
    [[nodiscard]] const std::unordered_map<std::string, Value>& globals() const noexcept {
        return globals_;
    }

private:
    ResourceLimits         limits_;
    size_t                 instructions_executed_{0};
    std::vector<Value>     stack_;
    std::vector<CallFrame> frames_;
    std::unordered_map<std::string, Value> globals_;
    std::function<void(std::string_view)>  output_handler_;

    void push(Value value);
    Value pop();
    Value peek(size_t distance = 0) const;

    bool call(Value::FunctionPtr function, uint8_t arg_count);
    bool call_value(const Value& callee, uint8_t arg_count);

    void print_output(std::string_view text);
};

} // namespace luna::vm
