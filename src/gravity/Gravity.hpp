#pragma once
// Top-Down Operator Precedence (Pratt) parser binding-power definitions.
#include "../ast/AST.hpp"
#include "../lexer/Token.hpp"
#include <cstdint>

namespace luna::gravity {

using lexer::TokenType;

// Binding powers for Pratt expression parsing (Gravity Engine)
enum class BindingPower : int {
    None       = 0,
    LogicalOr  = 1,   // or
    LogicalAnd = 2,   // and
    Equality   = 3,   // == ~=
    Comparison = 4,   // < <= > >=
    Addition   = 5,   // + -
    Multiply   = 6,   // * /
    Modulo     = 7,   // %
    Exponent   = 8,   // ^ (right-associative)
    Unary      = 9,   // - ~ (prefix)
};

class Gravity {
public:
    [[nodiscard]] static int left_binding_power(TokenType type) noexcept;
    [[nodiscard]] static int right_binding_power(TokenType type) noexcept;
};

} // namespace luna::gravity
