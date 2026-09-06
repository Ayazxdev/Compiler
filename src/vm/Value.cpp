// Dynamic Value operations implementation for the Gravity Bytecode VM.
#include "Value.hpp"
#include <cmath>
#include <format>
#include <sstream>
#include <stdexcept>

namespace luna::vm {

namespace {

std::string format_double(double d, bool is_concat) {
    if (std::isnan(d)) return "nan";
    if (std::isinf(d)) return d > 0 ? "inf" : "-inf";

    // If whole integer
    if (std::floor(d) == d && std::abs(d) < 1e15) {
        if (is_concat) {
            // Luna string representation for float whole number during concatenation is "1.0"
            return std::format("{:.1f}", d);
        }
        return std::to_string(static_cast<long long>(d));
    }
    // Format double cleanly
    std::string s = std::format("{}", d);
    return s;
}

} // namespace

std::string Value::to_string(bool is_concat) const {
    switch (type_) {
        case ValueType::Null:     return "null";
        case ValueType::Bool:     return as_bool() ? "true" : "false";
        case ValueType::Number:   return format_double(as_number(), is_concat);
        case ValueType::String:   return as_string();
        case ValueType::Function: return std::format("<fn {}>", as_function()->name);
    }
    return "null";
}

std::string Value::stringify() const {
    switch (type_) {
        case ValueType::Null:     return "null";
        case ValueType::Bool:     return as_bool() ? "true" : "false";
        case ValueType::Number:   return format_double(as_number(), false);
        case ValueType::String:   return as_string();
        case ValueType::Function: return std::format("<fn {}>", as_function()->name);
    }
    return "null";
}

bool Value::operator==(const Value& other) const {
    if (type_ != other.type_) return false;
    switch (type_) {
        case ValueType::Null:     return true;
        case ValueType::Bool:     return as_bool() == other.as_bool();
        case ValueType::Number:   return as_number() == other.as_number();
        case ValueType::String:   return as_string() == other.as_string();
        case ValueType::Function: return as_function().get() == other.as_function().get();
    }
    return false;
}

bool Value::operator<(const Value& other) const {
    if (is_number() && other.is_number()) {
        return as_number() < other.as_number();
    }
    if (is_string() && other.is_string()) {
        return as_string() < other.as_string();
    }
    throw std::runtime_error("Invalid operands for '<'.");
}

bool Value::operator<=(const Value& other) const {
    if (is_number() && other.is_number()) {
        return as_number() <= other.as_number();
    }
    if (is_string() && other.is_string()) {
        return as_string() <= other.as_string();
    }
    throw std::runtime_error("Invalid operands for '<='.");
}

bool Value::operator>(const Value& other) const {
    if (is_number() && other.is_number()) {
        return as_number() > other.as_number();
    }
    if (is_string() && other.is_string()) {
        return as_string() > other.as_string();
    }
    throw std::runtime_error("Invalid operands for '>'.");
}

bool Value::operator>=(const Value& other) const {
    if (is_number() && other.is_number()) {
        return as_number() >= other.as_number();
    }
    if (is_string() && other.is_string()) {
        return as_string() >= other.as_string();
    }
    throw std::runtime_error("Invalid operands for '>='.");
}

Value Value::operator+(const Value& other) const {
    if (is_number() && other.is_number()) {
        return Value(as_number() + other.as_number());
    }
    if (is_string() || other.is_string()) {
        return Value(to_string(true) + other.to_string(true));
    }
    throw std::runtime_error("Invalid operands for '+'.");
}

Value Value::operator-(const Value& other) const {
    if (is_number() && other.is_number()) {
        return Value(as_number() - other.as_number());
    }
    throw std::runtime_error("Invalid operands for '-'.");
}

Value Value::operator*(const Value& other) const {
    if (is_number() && other.is_number()) {
        return Value(as_number() * other.as_number());
    }
    throw std::runtime_error("Invalid operands for '*'.");
}

Value Value::operator/(const Value& other) const {
    if (is_number() && other.is_number()) {
        if (other.as_number() == 0.0) {
            throw std::runtime_error("Division by zero.");
        }
        return Value(as_number() / other.as_number());
    }
    throw std::runtime_error("Invalid operands for '/'.");
}

Value Value::operator%(const Value& other) const {
    if (is_number() && other.is_number()) {
        if (other.as_number() == 0.0) {
            throw std::runtime_error("Modulo by zero.");
        }
        return Value(std::fmod(as_number(), other.as_number()));
    }
    throw std::runtime_error("Invalid operands for '%'.");
}

Value Value::pow(const Value& other) const {
    if (is_number() && other.is_number()) {
        return Value(std::pow(as_number(), other.as_number()));
    }
    throw std::runtime_error("Invalid operands for '^'.");
}

Value Value::operator-() const {
    if (is_number()) {
        return Value(-as_number());
    }
    throw std::runtime_error("Invalid operand for unary '-'.");
}

} // namespace luna::vm
