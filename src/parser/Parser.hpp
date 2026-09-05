#pragma once
// Unified parser for Luna: recursive-descent for statements, Top Down operator-precedence (Pratt) for expressions.
#include "../ast/AST.hpp"
#include "../gravity/Gravity.hpp"
#include "../lexer/Lexer.hpp"
#include "../memory/Arena.hpp"
#include <stdexcept>
#include <string>

namespace luna::parser {

using namespace luna::lexer;
using namespace luna::ast;
using luna::gravity::BindingPower;
using luna::gravity::Gravity;

// ParseError
struct ParseError : std::runtime_error {
    std::size_t line;
    ParseError(const std::string& msg, std::size_t ln)
        : std::runtime_error(msg), line(ln) {}
};

// Parser
class Parser {
public:
    /// `source` must outlive the Parser. `arena` owns all allocated AST nodes.
    Parser(std::string_view source, luna::memory::Arena& arena);

    /// Entry point — parse a full program (list of statements).
    [[nodiscard]] Stmts* parse();

private:
    Lexer                  lexer_;
    luna::memory::Arena&   arena_;
    Token                  current_{};  // look-ahead token (already consumed from lexer)
    Token                  previous_{}; // most recently consumed token

    // Lexer interface
    Token  advance();                         // consume current_ -> previous_
    Token  peek() const noexcept;             // non-consuming look-ahead
    bool   is_next(TokenType t) const noexcept;
    bool   match(TokenType t);                // consume if matches
    Token  expect(TokenType t, std::string_view what); // consume or throw

    // Gravity Pratt expression parsing
    Expr  parse_expr(BindingPower min_bp = BindingPower::None);
    Expr  parse_prefix();    // nud: literals, identifiers, unary, grouping
    Expr  parse_infix(Expr left, Token op); // led: binary / logical ops

    // Statement parsing (recursive-descent)
    Stmts* parse_stmts(TokenType stop1 = TokenType::Eof,
                       TokenType stop2 = TokenType::Eof,
                       TokenType stop3 = TokenType::Eof);
    Stmt  parse_stmt();
    Stmt  parse_print(bool newline);
    Stmt  parse_if();
    Stmt  parse_while();
    Stmt  parse_for();
    Stmt  parse_func_decl();
    Stmt  parse_ret();
    Stmt  parse_assign_or_call(); // identifier -> assignment or function-call-stmt

    // Arena helpers
    template<typename T, typename... A>
    T* make(A&&... a) { return arena_.alloc<T>(std::forward<A>(a)...); }
};

} // namespace luna::parser
