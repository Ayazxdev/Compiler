// Zero-copy lexical scanner implementation for the Luna programming language.
// Every Token.lexeme is a string_view slice into the source buffer.
#include "Lexer.hpp"
#include <cctype>
#include <format>

namespace luna::lexer {

// Construction
Lexer::Lexer(std::string_view source) noexcept : src_(source) {}

// Primitives
bool Lexer::at_end() const noexcept { return curr_ >= src_.size(); }

char Lexer::peek() const noexcept {
    return at_end() ? '\0' : src_[curr_];
}

char Lexer::peek2() const noexcept {
    return (curr_ + 1 >= src_.size()) ? '\0' : src_[curr_ + 1];
}

char Lexer::advance() noexcept {
    return src_[curr_++];
}

bool Lexer::match(char expected) noexcept {
    if (at_end() || src_[curr_] != expected) return false;
    ++curr_;
    return true;
}

// Token factory — lexeme is a view into [start_, curr_)
Token Lexer::make(TokenType t) const noexcept {
    return Token{t, src_.substr(start_, curr_ - start_), line_};
}

// Number parsing (integers and floating-point literals)
Token Lexer::handle_number() {
    while (std::isdigit(static_cast<unsigned char>(peek())))
        advance();
    // Check for decimal: peek == '.' AND lookahead is digit
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek2()))) {
        advance(); // consume '.'
        while (std::isdigit(static_cast<unsigned char>(peek())))
            advance();
        return make(TokenType::Float);
    }
    return make(TokenType::Integer);
}

// String literal parsing (preserves enclosing quote delimiters)
Token Lexer::handle_string(char quote) {
    while (!at_end() && peek() != quote) {
        if (peek() == '\\') {
            advance(); // consume '\\'
            if (!at_end()) {
                if (peek() == '\n') ++line_;
                advance(); // consume escaped char
            }
            continue;
        }
        if (peek() == '\n') ++line_;
        advance();
    }
    if (at_end())
        throw LexError("Unterminated string.", line_);
    advance(); // consume closing quote
    return make(TokenType::String);
}

// Identifier and keyword scanning
Token Lexer::handle_identifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')
        advance();
    std::string_view text = src_.substr(start_, curr_ - start_);
    auto it = kKeywords.find(text);
    return (it != kKeywords.end()) ? make(it->second) : make(TokenType::Identifier);
}

// Main scan loop: returns next token on demand (pull-based lexing)
Token Lexer::next() {
    // Skip whitespace and comments (-- to end of line), updating line_
    while (!at_end()) {
        char c = peek();
        if (c == '\n') { ++line_; advance(); continue; }
        if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }
        // -- comment
        if (c == '-' && peek2() == '-') {
            while (!at_end() && peek() != '\n') advance();
            continue;
        }
        break;
    }
    if (at_end()) { start_ = curr_; return make(TokenType::Eof); }

    start_ = curr_;
    char ch = advance();

    switch (ch) {
        // Single-character
        case '(': return make(TokenType::LParen);
        case ')': return make(TokenType::RParen);
        case '{': return make(TokenType::LCurly);
        case '}': return make(TokenType::RCurly);
        case '[': return make(TokenType::LSquar);
        case ']': return make(TokenType::RSquar);
        case '.': return make(TokenType::Dot);
        case ',': return make(TokenType::Comma);
        case '+': return make(TokenType::Plus);
        case '*': return make(TokenType::Star);
        case '^': return make(TokenType::Caret);
        case '/': return make(TokenType::Slash);
        case ';': return make(TokenType::Semicolon);
        case '?': return make(TokenType::Question);
        case '%': return make(TokenType::Mod);
        // Minus: could be operator (comment already consumed above)
        case '-': return make(TokenType::Minus);
        // = or ==
        case '=': return make(match('=') ? TokenType::EqEq : TokenType::Eq);
        // ~ or ~=
        case '~': return make(match('=') ? TokenType::Ne : TokenType::Tilde);
        // < or <= or <<
        case '<':
            if (match('=')) return make(TokenType::Le);
            if (match('<')) return make(TokenType::LtLt);
            return make(TokenType::Lt);
        // > or >= or >>
        case '>':
            if (match('=')) return make(TokenType::Ge);
            if (match('>')) return make(TokenType::GtGt);
            return make(TokenType::Gt);
        // : or :=
        case ':': return make(match('=') ? TokenType::Assign : TokenType::Colon);
        // String literals
        case '"': case '\'': return handle_string(ch);
        default: break;
    }

    if (std::isdigit(static_cast<unsigned char>(ch))) return handle_number();
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') return handle_identifier();

    throw LexError(
        std::format("Unexpected character '{}'.", ch), line_);
}

} // namespace luna::lexer
