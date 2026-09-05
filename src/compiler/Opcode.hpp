#pragma once
// Gravity Bytecode Virtual Machine Instruction Set Architecture (ISA).
#include <cstdint>
#include <string_view>

namespace luna::compiler {

enum class Opcode : uint8_t {
    // Constant / Literal loading
    Constant,       // [idx: uint16_t] -> push constants[idx]
    Null,           //                 -> push null
    True,           //                 -> push true
    False,          //                 -> push false

    // Stack manipulation
    Pop,            // pop and discard top of stack
    Dup,            // duplicate top of stack

    // Global variables
    GetGlobal,      // [name_idx: uint16_t] -> push globals[constants[name_idx]]
    SetGlobal,      // [name_idx: uint16_t] -> globals[constants[name_idx]] = peek()

    // Local variables
    GetLocal,       // [slot: uint16_t] -> push stack[frame.base + slot]
    SetLocal,       // [slot: uint16_t] -> stack[frame.base + slot] = peek()

    // Comparisons
    Equal,          // a, b -> (a == b)
    NotEqual,       // a, b -> (a != b)
    Greater,        // a, b -> (a > b)
    GreaterEqual,   // a, b -> (a >= b)
    Less,           // a, b -> (a < b)
    LessEqual,      // a, b -> (a <= b)

    // Arithmetic
    Add,            // a, b -> (a + b) (or string concatenation)
    Sub,            // a, b -> (a - b)
    Mul,            // a, b -> (a * b)
    Div,            // a, b -> (a / b)
    Mod,            // a, b -> (a % b)
    Exp,            // a, b -> (a ^ b)
    Negate,         // a -> -a

    // Logic & Bitwise
    Not,            // a -> !a
    BitAnd,         // a, b -> (a & b)
    BitOr,          // a, b -> (a | b)
    BitXor,         // a, b -> (a ^ b)

    // Control flow
    Jump,           // [offset: uint16_t] -> pc += offset
    JumpIfFalse,    // [offset: uint16_t] -> if !pop(): pc += offset
    Loop,           // [offset: uint16_t] -> pc -= offset

    // Functions
    Call,           // [argc: uint8_t] -> call fn with argc arguments
    Return,         // -> return top of stack from current function
    DefineFunc,     // [name_idx: uint16_t, fn_idx: uint16_t]

    // I/O & Sandbox
    Print,          // pop and print without newline
    Println,        // pop and print with newline

    // Sentinel
    Halt            // stop VM execution
};

[[nodiscard]] inline std::string_view opcode_name(Opcode op) noexcept {
    switch (op) {
        case Opcode::Constant:     return "OP_CONSTANT";
        case Opcode::Null:         return "OP_NULL";
        case Opcode::True:         return "OP_TRUE";
        case Opcode::False:        return "OP_FALSE";
        case Opcode::Pop:          return "OP_POP";
        case Opcode::Dup:          return "OP_DUP";
        case Opcode::GetGlobal:    return "OP_GET_GLOBAL";
        case Opcode::SetGlobal:    return "OP_SET_GLOBAL";
        case Opcode::GetLocal:     return "OP_GET_LOCAL";
        case Opcode::SetLocal:     return "OP_SET_LOCAL";
        case Opcode::Equal:        return "OP_EQUAL";
        case Opcode::NotEqual:     return "OP_NOT_EQUAL";
        case Opcode::Greater:      return "OP_GREATER";
        case Opcode::GreaterEqual: return "OP_GREATER_EQUAL";
        case Opcode::Less:         return "OP_LESS";
        case Opcode::LessEqual:    return "OP_LESS_EQUAL";
        case Opcode::Add:          return "OP_ADD";
        case Opcode::Sub:          return "OP_SUB";
        case Opcode::Mul:          return "OP_MUL";
        case Opcode::Div:          return "OP_DIV";
        case Opcode::Mod:          return "OP_MOD";
        case Opcode::Exp:          return "OP_EXP";
        case Opcode::Negate:       return "OP_NEGATE";
        case Opcode::Not:          return "OP_NOT";
        case Opcode::BitAnd:       return "OP_BIT_AND";
        case Opcode::BitOr:        return "OP_BIT_OR";
        case Opcode::BitXor:       return "OP_BIT_XOR";
        case Opcode::Jump:         return "OP_JUMP";
        case Opcode::JumpIfFalse:  return "OP_JUMP_IF_FALSE";
        case Opcode::Loop:         return "OP_LOOP";
        case Opcode::Call:         return "OP_CALL";
        case Opcode::Return:       return "OP_RETURN";
        case Opcode::DefineFunc:   return "OP_DEFINE_FUNC";
        case Opcode::Print:        return "OP_PRINT";
        case Opcode::Println:      return "OP_PRINTLN";
        case Opcode::Halt:         return "OP_HALT";
    }
    return "UNKNOWN_OP";
}

} // namespace luna::compiler
