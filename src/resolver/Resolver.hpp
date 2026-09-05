#pragma once
// Semantic Analysis and Scope Resolution pass for Luna programs.
#include "../ast/AST.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace luna::resolver {

enum class ScopeType {
    Global,
    Function,
    Block
};

struct Local {
    std::string name;
    int         depth{0};
    uint16_t    slot{0};
};

struct FunctionScope {
    std::string        name;
    ScopeType          type{ScopeType::Global};
    std::vector<Local> locals;
    int                scope_depth{0};
    uint16_t           local_count{0};
};

struct ResolveError : std::runtime_error {
    std::size_t line;
    ResolveError(const std::string& msg, std::size_t ln)
        : std::runtime_error(msg), line(ln) {}
};

class Resolver {
public:
    Resolver() = default;

    void resolve(ast::Stmts* program);
    void resolve(const ast::Stmt& stmt);
    void resolve(const ast::Expr& expr);

    // Expose resolved slot mappings to the compiler
    [[nodiscard]] std::unordered_map<const ast::Identifier*, uint16_t> locals_map() const {
        return resolved_locals_;
    }

private:
    std::vector<FunctionScope> scopes_;
    std::unordered_map<const ast::Identifier*, uint16_t> resolved_locals_;

    void begin_scope();
    void end_scope();
    void declare(std::string_view name, std::size_t line);
    void resolve_local(const ast::Identifier* expr);
};

} // namespace luna::resolver
