// Pratt operator precedence tables and associativity rules.
#include "Gravity.hpp"

namespace luna::gravity {

int Gravity::left_binding_power(TokenType type) noexcept {
    switch (type) {
        case TokenType::Or:     return static_cast<int>(BindingPower::LogicalOr);
        case TokenType::And:    return static_cast<int>(BindingPower::LogicalAnd);
        case TokenType::EqEq:
        case TokenType::Ne:     return static_cast<int>(BindingPower::Equality);
        case TokenType::Lt:
        case TokenType::Le:
        case TokenType::Gt:
        case TokenType::Ge:     return static_cast<int>(BindingPower::Comparison);
        case TokenType::Plus:
        case TokenType::Minus:  return static_cast<int>(BindingPower::Addition);
        case TokenType::Star:
        case TokenType::Slash:  return static_cast<int>(BindingPower::Multiply);
        case TokenType::Mod:    return static_cast<int>(BindingPower::Modulo);
        case TokenType::Caret:  return static_cast<int>(BindingPower::Exponent);
        default:                return 0;
    }
}

int Gravity::right_binding_power(TokenType type) noexcept {
    // Exponentiation ^ is right-associative
    if (type == TokenType::Caret) {
        return static_cast<int>(BindingPower::Exponent);
    }
    return left_binding_power(type);
}

} // namespace luna::gravity
