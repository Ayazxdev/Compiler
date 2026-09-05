#pragma once
// Token definitions for the Luna programming language.
// Maps all token types and keywords used by the lexer and parser.
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace luna::lexer {

// Token types
// Order mirrors tokens.py for easy cross-referencing.
enum class TokenType : std::uint8_t {
    // Single-character tokens
    LParen,     // (
    RParen,     // )
    LCurly,     // {
    RCurly,     // }
    LSquar,     // [
    RSquar,     // ]
    Comma,      // ,
    Dot,        // .
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /
    Caret,      // ^
    Mod,        // %
    Colon,      // :
    Semicolon,  // ;
    Question,   // ?
    Tilde,      // ~ (unary NOT when not followed by =)
    Gt,         // >
    Lt,         // <
    Eq,         // =
    // Two-character tokens
    Ge,         // >=
    Le,         // <=
    Ne,         // ~=
    EqEq,       // ==
    Assign,     // :=
    GtGt,       // >>
    LtLt,       // <<
    // Literals
    Identifier,
    String,
    Integer,
    Float,
    // Keywords
    If,         // if
    Then,       // then
    Else,       // else
    True_,      // true
    False_,     // false
    And,        // and
    Or,         // or
    While,      // while
    Do,         // do
    For,        // for
    Func,       // func
    Null,       // null
    End,        // end
    Print,      // print
    Println,    // println
    Ret,        // ret
    Local,      // local variable declaration
    // Sentinel
    Eof
};

// Token struct
struct Token {
    TokenType        type{TokenType::Eof};
    std::string_view lexeme{};
    std::size_t      line{1};
};

// Keyword table
// NOTE: declared inline so every TU sees the same definition.
inline const std::unordered_map<std::string_view, TokenType> kKeywords = {
    {"if",      TokenType::If},
    {"else",    TokenType::Else},
    {"then",    TokenType::Then},
    {"true",    TokenType::True_},
    {"false",   TokenType::False_},
    {"and",     TokenType::And},
    {"or",      TokenType::Or},
    {"while",   TokenType::While},
    {"do",      TokenType::Do},
    {"for",     TokenType::For},
    {"func",    TokenType::Func},
    {"null",    TokenType::Null},
    {"end",     TokenType::End},
    {"print",   TokenType::Print},
    {"println", TokenType::Println},
    {"ret",     TokenType::Ret},
    {"local",   TokenType::Local},
};

// Debug helper
[[nodiscard]] inline std::string_view token_type_name(TokenType t) noexcept {
    switch (t) {
        case TokenType::LParen:     return "LParen";
        case TokenType::RParen:     return "RParen";
        case TokenType::LCurly:     return "LCurly";
        case TokenType::RCurly:     return "RCurly";
        case TokenType::LSquar:     return "LSquar";
        case TokenType::RSquar:     return "RSquar";
        case TokenType::Comma:      return "Comma";
        case TokenType::Dot:        return "Dot";
        case TokenType::Plus:       return "Plus";
        case TokenType::Minus:      return "Minus";
        case TokenType::Star:       return "Star";
        case TokenType::Slash:      return "Slash";
        case TokenType::Caret:      return "Caret";
        case TokenType::Mod:        return "Mod";
        case TokenType::Colon:      return "Colon";
        case TokenType::Semicolon:  return "Semicolon";
        case TokenType::Question:   return "Question";
        case TokenType::Tilde:      return "Tilde";
        case TokenType::Gt:         return "Gt";
        case TokenType::Lt:         return "Lt";
        case TokenType::Eq:         return "Eq";
        case TokenType::Ge:         return "Ge";
        case TokenType::Le:         return "Le";
        case TokenType::Ne:         return "Ne";
        case TokenType::EqEq:       return "EqEq";
        case TokenType::Assign:     return "Assign";
        case TokenType::GtGt:       return "GtGt";
        case TokenType::LtLt:       return "LtLt";
        case TokenType::Identifier: return "Identifier";
        case TokenType::String:     return "String";
        case TokenType::Integer:    return "Integer";
        case TokenType::Float:      return "Float";
        case TokenType::If:         return "If";
        case TokenType::Then:       return "Then";
        case TokenType::Else:       return "Else";
        case TokenType::True_:      return "True_";
        case TokenType::False_:     return "False_";
        case TokenType::And:        return "And";
        case TokenType::Or:         return "Or";
        case TokenType::While:      return "While";
        case TokenType::Do:         return "Do";
        case TokenType::For:        return "For";
        case TokenType::Func:       return "Func";
        case TokenType::Null:       return "Null";
        case TokenType::End:        return "End";
        case TokenType::Print:      return "Print";
        case TokenType::Println:    return "Println";
        case TokenType::Ret:        return "Ret";
        case TokenType::Local:      return "Local";
        case TokenType::Eof:        return "Eof";
    }
    return "Unknown";
}

} // namespace luna::lexer
