#pragma once
// AST Hierarchy for the Luna language using C++26 std::variant and value semantics.
// Node allocations are managed via Arena for O(1) allocation/deallocation.
#include "../lexer/Token.hpp"
#include <cstddef>
#include <string_view>
#include <vector>
#include <variant>
#include <optional>

namespace luna::ast {

using luna::lexer::Token;

// Base node
struct Node {
    std::size_t line{0};
};

// Forward declarations
struct IntegerLit; struct FloatLit; struct BoolLit; struct NullLit;
struct StringLit; struct Identifier; struct Grouping; struct UnOp;
struct BinOp; struct LogicalOp; struct FuncCall;

// Expression Variant
using Expr = std::variant<
    IntegerLit*, FloatLit*, BoolLit*, NullLit*, StringLit*,
    Identifier*, Grouping*, UnOp*, BinOp*, LogicalOp*, FuncCall*
>;

struct IntegerLit : Node { long long value{0}; };
struct FloatLit   : Node { double value{0.0}; };
struct BoolLit    : Node { bool value{false}; };
struct NullLit    : Node {};
struct StringLit  : Node { std::string_view value; };
struct Identifier : Node { std::string_view name; };
struct Grouping   : Node { Expr inner; };
struct UnOp       : Node { Token op; Expr operand; };
struct BinOp      : Node { Token op; Expr left; Expr right; };
struct LogicalOp  : Node { Token op; Expr left; Expr right; };
struct FuncCall   : Node { std::string_view name; std::vector<Expr> args; };

// Statement Forward declarations
struct PrintStmt; struct IfStmt; struct WhileStmt; struct ForStmt;
struct FuncDecl; struct RetStmt; struct Assignment; struct FuncCallStmt;

// Statement Variant
using Stmt = std::variant<
    PrintStmt*, IfStmt*, WhileStmt*, ForStmt*, FuncDecl*,
    RetStmt*, Assignment*, FuncCallStmt*
>;

struct Stmts : Node { std::vector<Stmt> stmts; };
struct Param : Node { std::string_view name; };

struct PrintStmt : Node { Expr value; bool newline{false}; };
struct IfStmt    : Node { Expr test; Stmts* then_body{nullptr}; Stmts* else_body{nullptr}; };
struct WhileStmt : Node { Expr test; Stmts* body{nullptr}; };
struct ForStmt   : Node { Identifier* ident{nullptr}; Expr start; Expr end; std::optional<Expr> step; Stmts* body{nullptr}; };
struct FuncDecl  : Node { std::string_view name; std::vector<Param*> params; Stmts* body{nullptr}; };
struct RetStmt   : Node { Expr value; };
struct Assignment: Node { Expr left; Expr right; };
struct FuncCallStmt : Node { FuncCall* call{nullptr}; };

// Visitor Helper (Overloaded lambda pattern)
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace luna::ast
