#pragma once
// Zero-copy, std::string_view-based lexical scanner for the Luna language.
#include "Token.hpp"
#include <stdexcept>
#include <string>

namespace luna::lexer {

// LexError
struct LexError : std::runtime_error {
    std::size_t line;
    LexError(const std::string& msg, std::size_t ln)
        : std::runtime_error(msg), line(ln) {}
};

// Lexer
class Lexer {
public:
    /// Construct from source text.  The caller must keep `source` alive for
    /// the lifetime of the Lexer (and any Tokens produced from it).
    explicit Lexer(std::string_view source) noexcept;

    /// Scan and return the next token.  Returns Eof once the source is exhausted.
    [[nodiscard]] Token next();

    /// True if the scanner has reached the end of source.
    [[nodiscard]] bool at_end() const noexcept;

private:
    std::string_view src_;
    std::size_t      start_{0};
    std::size_t      curr_{0};
    std::size_t      line_{1};

    // Scanning primitives
    char advance()               noexcept;
    [[nodiscard]] char peek()    const noexcept;
    [[nodiscard]] char peek2()   const noexcept; // 2-character lookahead
    bool match(char expected)    noexcept;

    // Token factories
    [[nodiscard]] Token make(TokenType t)       const noexcept;
    [[nodiscard]] Token handle_number();
    [[nodiscard]] Token handle_string(char quote);
    [[nodiscard]] Token handle_identifier();
};

} // namespace luna::lexer
