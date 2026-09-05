// Unified recursive-descent + Pratt expression parser implementation for Luna.
#include "Parser.hpp"
#include <charconv>
#include <format>
#include <string>

namespace luna::parser {

using TT = TokenType;

// Construction / initialization
Parser::Parser(std::string_view source, luna::memory::Arena& arena)
    : lexer_(source), arena_(arena) {
    // Prime the look-ahead
    current_ = lexer_.next();
}

// Lexer interface helpers
Token Parser::advance() {
    previous_ = current_;
    current_  = lexer_.at_end() ? Token{TT::Eof, {}, previous_.line} : lexer_.next();
    return previous_;
}

Token Parser::peek() const noexcept { return current_; }

bool Parser::is_next(TT t) const noexcept { return current_.type == t; }

bool Parser::match(TT t) {
    if (!is_next(t)) return false;
    advance();
    return true;
}

Token Parser::expect(TT t, std::string_view what) {
    if (is_next(t)) return advance();
    throw ParseError(
        std::format("Expected {}, found '{}' at line {}.",
            what, current_.lexeme.empty() ? "<eof>" : current_.lexeme, current_.line),
        current_.line);
}

// Pratt expression parser — the Gravity core
Expr Parser::parse_prefix() {
    Token t = advance();
    switch (t.type) {
        case TT::Integer: {
            auto* n   = make<IntegerLit>();
            n->line   = t.line;
            std::from_chars(t.lexeme.data(), t.lexeme.data() + t.lexeme.size(), n->value);
            return n;
        }
        case TT::Float: {
            auto* n   = make<FloatLit>();
            n->line   = t.line;
            n->value  = std::stod(std::string(t.lexeme));
            return n;
        }
        case TT::True_: {
            auto* n   = make<BoolLit>();
            n->line   = t.line;
            n->value  = true;
            return n;
        }
        case TT::False_: {
            auto* n   = make<BoolLit>();
            n->line   = t.line;
            n->value  = false;
            return n;
        }
        case TT::Null: {
            auto* n = make<NullLit>();
            n->line = t.line;
            return n;
        }
        case TT::String: {
            auto* n   = make<StringLit>();
            n->line   = t.line;
            // Strip surrounding quotes
            n->value  = t.lexeme.size() >= 2
                      ? t.lexeme.substr(1, t.lexeme.size() - 2)
                      : t.lexeme;
            return n;
        }
        case TT::LParen: {
            Expr inner = parse_expr(BindingPower::None);
            expect(TT::RParen, "')'");
            auto* g   = make<Grouping>();
            g->line   = t.line;
            g->inner  = inner;
            return g;
        }
        case TT::Minus:
        case TT::Plus:
        case TT::Tilde: {
            Expr operand  = parse_expr(BindingPower::Unary);
            auto* u       = make<UnOp>();
            u->line       = t.line;
            u->op         = t;
            u->operand    = operand;
            return u;
        }
        case TT::Identifier: {
            if (is_next(TT::LParen)) {
                advance(); // consume '('
                auto* call  = make<FuncCall>();
                call->line  = t.line;
                call->name  = t.lexeme;
                if (!is_next(TT::RParen)) {
                    call->args.push_back(parse_expr(BindingPower::None));
                    while (match(TT::Comma))
                        call->args.push_back(parse_expr(BindingPower::None));
                }
                expect(TT::RParen, "')'");
                return call;
            }
            auto* id  = make<Identifier>();
            id->line  = t.line;
            id->name  = t.lexeme;
            return id;
        }
        default:
            throw ParseError(
                std::format("Unexpected token '{}' in expression at line {}.",
                    t.lexeme.empty() ? "<eof>" : t.lexeme, t.line),
                t.line);
    }
}

Expr Parser::parse_infix(Expr left, Token op) {
    if (op.type == TT::And || op.type == TT::Or) {
        Expr right   = parse_expr(static_cast<BindingPower>(Gravity::right_binding_power(op.type)));
        auto* log    = make<LogicalOp>();
        log->line    = op.line;
        log->op      = op;
        log->left    = left;
        log->right   = right;
        return log;
    }
    Expr right   = parse_expr(static_cast<BindingPower>(Gravity::right_binding_power(op.type)));
    auto* bin    = make<BinOp>();
    bin->line    = op.line;
    bin->op      = op;
    bin->left    = left;
    bin->right   = right;
    return bin;
}

Expr Parser::parse_expr(BindingPower min_bp) {
    Expr left = parse_prefix();
    while (true) {
        int lbp = Gravity::left_binding_power(current_.type);
        if (lbp <= static_cast<int>(min_bp)) break;
        Token op = advance();
        left = parse_infix(left, op);
    }
    return left;
}

// Statement parsers — recursive descent
Stmts* Parser::parse_stmts(TT stop1, TT stop2, TT stop3) {
    auto* ss = make<Stmts>();
    while (!is_next(TT::Eof)
           && !is_next(stop1)
           && !is_next(stop2)
           && !is_next(stop3)) {
        ss->stmts.push_back(parse_stmt());
    }
    return ss;
}

Stmt Parser::parse_print(bool newline) {
    advance(); // consume 'print'/'println'
    auto* p    = make<PrintStmt>();
    p->line    = previous_.line;
    p->value   = parse_expr(BindingPower::None);
    p->newline = newline;
    return p;
}

Stmt Parser::parse_if() {
    Token kw = advance(); // consume 'if'
    Expr test = parse_expr(BindingPower::None);
    expect(TT::Then, "'then'");
    Stmts* then_body = parse_stmts(TT::Else, TT::End);
    Stmts* else_body = nullptr;
    if (match(TT::Else))
        else_body = parse_stmts(TT::End);
    expect(TT::End, "'end'");
    auto* s        = make<IfStmt>();
    s->line        = kw.line;
    s->test        = test;
    s->then_body   = then_body;
    s->else_body   = else_body;
    return s;
}

Stmt Parser::parse_while() {
    Token kw = advance(); // consume 'while'
    Expr test = parse_expr(BindingPower::None);
    expect(TT::Do, "'do'");
    Stmts* body = parse_stmts(TT::End);
    expect(TT::End, "'end'");
    auto* s   = make<WhileStmt>();
    s->line   = kw.line;
    s->test   = test;
    s->body   = body;
    return s;
}

Stmt Parser::parse_for() {
    Token kw = advance(); // consume 'for'
    Token id_tok = expect(TT::Identifier, "loop variable");
    auto* id     = make<Identifier>();
    id->line     = id_tok.line;
    id->name     = id_tok.lexeme;
    expect(TT::Assign, "':='");
    Expr start = parse_expr(BindingPower::None);
    expect(TT::Comma, "','");
    Expr end   = parse_expr(BindingPower::None);
    std::optional<Expr> step = std::nullopt;
    if (match(TT::Comma))
        step = parse_expr(BindingPower::None);
    expect(TT::Do, "'do'");
    Stmts* body = parse_stmts(TT::End);
    expect(TT::End, "'end'");
    auto* s     = make<ForStmt>();
    s->line     = kw.line;
    s->ident    = id;
    s->start    = start;
    s->end      = end;
    s->step     = step;
    s->body     = body;
    return s;
}

Stmt Parser::parse_func_decl() {
    Token kw  = advance(); // consume 'func'
    Token name = expect(TT::Identifier, "function name");
    expect(TT::LParen, "'('");
    auto* fd   = make<FuncDecl>();
    fd->line   = kw.line;
    fd->name   = name.lexeme;
    if (!is_next(TT::RParen)) {
        do {
            Token p_tok = expect(TT::Identifier, "parameter name");
            auto* p     = make<Param>();
            p->line     = p_tok.line;
            p->name     = p_tok.lexeme;
            fd->params.push_back(p);
        } while (match(TT::Comma));
    }
    expect(TT::RParen, "')'");
    fd->body = parse_stmts(TT::End);
    expect(TT::End, "'end'");
    return fd;
}

Stmt Parser::parse_ret() {
    Token kw = advance(); // consume 'ret'
    auto* r  = make<RetStmt>();
    r->line  = kw.line;
    r->value = parse_expr(BindingPower::None);
    return r;
}

Stmt Parser::parse_assign_or_call() {
    Expr lhs = parse_expr(BindingPower::None);
    if (match(TT::Assign)) {
        Expr rhs    = parse_expr(BindingPower::None);
        auto* a     = make<Assignment>();
        a->line     = previous_.line;
        a->left     = lhs;
        a->right    = rhs;
        return a;
    }
    if (std::holds_alternative<FuncCall*>(lhs)) {
        auto* fc   = std::get<FuncCall*>(lhs);
        auto* fcs  = make<FuncCallStmt>();
        fcs->call  = fc;
        fcs->line  = fc->line;
        return fcs;
    }
    throw ParseError(
        std::format("Expected ':=' or function call at line {}.", previous_.line),
        previous_.line);
}

Stmt Parser::parse_stmt() {
    switch (current_.type) {
        case TT::Print:   return parse_print(false);
        case TT::Println: return parse_print(true);
        case TT::If:      return parse_if();
        case TT::While:   return parse_while();
        case TT::For:     return parse_for();
        case TT::Func:    return parse_func_decl();
        case TT::Ret:     return parse_ret();
        default:          return parse_assign_or_call();
    }
}

Stmts* Parser::parse() {
    Stmts* program = parse_stmts(TT::Eof);
    expect(TT::Eof, "end of file");
    return program;
}

} // namespace luna::parser
