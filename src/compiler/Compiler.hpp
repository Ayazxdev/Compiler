#pragma once
// AST-to-Bytecode Compiler for Luna emitting instructions for the Gravity VM.
#include "Chunk.hpp"
#include "../ast/AST.hpp"
#include "../vm/Value.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace luna::compiler {

struct LocalVar {
    std::string name;
    int         depth{0};
};

enum class FunctionType {
    Script,
    Function
};

class Compiler {
public:
    explicit Compiler(FunctionType type = FunctionType::Script, std::string name = "<script>");

    std::shared_ptr<Chunk> compile(ast::Stmts* program);
    
    void compile(const ast::Stmt& stmt);
    void compile(const ast::Expr& expr);

private:
    FunctionType              type_;
    std::string               name_;
    std::shared_ptr<Chunk>    chunk_;
    std::vector<LocalVar>     locals_;
    int                       scope_depth_{0};
    std::unordered_map<const ast::Identifier*, uint16_t> resolved_locals_;

    // Helpers
    Chunk& current_chunk() { return *chunk_; }
    void emit_byte(uint8_t byte, std::size_t line);
    void emit_bytes(uint8_t byte1, uint8_t byte2, std::size_t line);
    uint16_t make_constant(vm::Value value, std::size_t line);
    void emit_constant(vm::Value value, std::size_t line);

    // Control flow
    std::size_t emit_jump(uint8_t instruction, std::size_t line);
    void patch_jump(std::size_t offset);
    void emit_loop(std::size_t loop_start, std::size_t line);

    // Scoping
    void begin_scope();
    void end_scope(std::size_t line);
    void declare_variable(std::string_view name);
};

} // namespace luna::compiler
