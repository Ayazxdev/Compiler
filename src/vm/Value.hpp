#pragma once
// Dynamic runtime Value representation for the Gravity Bytecode Virtual Machine.
// Supports Numbers (64-bit IEEE 754 float), Booleans, Strings, Functions, Null.
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace luna::compiler {
struct Chunk;
}

namespace luna::vm {

enum class ValueType : uint8_t {
    Null,
    Bool,
    Number,
    String,
    Function
};

struct ObjFunction;

class Value {
public:
    using FunctionPtr = std::shared_ptr<ObjFunction>;

    Value() noexcept : type_(ValueType::Null), data_(std::monostate{}) {}
    explicit Value(bool b) noexcept : type_(ValueType::Bool), data_(b) {}
    explicit Value(double n) noexcept : type_(ValueType::Number), data_(n) {}
    explicit Value(int n) noexcept : type_(ValueType::Number), data_(static_cast<double>(n)) {}
    explicit Value(long long n) noexcept : type_(ValueType::Number), data_(static_cast<double>(n)) {}
    explicit Value(std::string s) : type_(ValueType::String), data_(std::move(s)) {}
    explicit Value(std::string_view sv) : type_(ValueType::String), data_(std::string(sv)) {}
    explicit Value(const char* s) : type_(ValueType::String), data_(std::string(s)) {}
    explicit Value(FunctionPtr fn) noexcept : type_(ValueType::Function), data_(std::move(fn)) {}

    // Type checks
    [[nodiscard]] ValueType type() const noexcept { return type_; }
    [[nodiscard]] bool is_null() const noexcept { return type_ == ValueType::Null; }
    [[nodiscard]] bool is_bool() const noexcept { return type_ == ValueType::Bool; }
    [[nodiscard]] bool is_number() const noexcept { return type_ == ValueType::Number; }
    [[nodiscard]] bool is_string() const noexcept { return type_ == ValueType::String; }
    [[nodiscard]] bool is_function() const noexcept { return type_ == ValueType::Function; }

    // Accessors
    [[nodiscard]] bool as_bool() const { return std::get<bool>(data_); }
    [[nodiscard]] double as_number() const { return std::get<double>(data_); }
    [[nodiscard]] const std::string& as_string() const { return std::get<std::string>(data_); }
    [[nodiscard]] std::string& as_string() { return std::get<std::string>(data_); }
    [[nodiscard]] const FunctionPtr& as_function() const { return std::get<FunctionPtr>(data_); }

    // Truthiness
    [[nodiscard]] bool is_truthy() const noexcept {
        if (is_null()) return false;
        if (is_bool()) return as_bool();
        if (is_number()) return as_number() != 0.0;
        if (is_string()) return !as_string().empty();
        return true;
    }

    // String formatting
    [[nodiscard]] std::string to_string(bool is_concat = false) const;
    [[nodiscard]] std::string stringify() const;

    // Comparisons
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;

    // Arithmetic
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    Value operator%(const Value& other) const;
    [[nodiscard]] Value pow(const Value& other) const;
    Value operator-() const;

private:
    ValueType type_{ValueType::Null};
    std::variant<std::monostate, bool, double, std::string, FunctionPtr> data_;
};

struct ObjFunction {
    std::string name;
    std::size_t arity{0};
    std::shared_ptr<luna::compiler::Chunk> chunk;

    ObjFunction(std::string n, std::size_t a, std::shared_ptr<luna::compiler::Chunk> c)
        : name(std::move(n)), arity(a), chunk(std::move(c)) {}
};

} // namespace luna::vm
