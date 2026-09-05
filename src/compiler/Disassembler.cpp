// Implementation of Gravity VM bytecode disassembler.
#include "Disassembler.hpp"
#include <format>
#include <iostream>

namespace luna::compiler {

namespace {

size_t simple_instruction(std::string_view name, size_t offset) {
    std::cout << std::format("{:<16}\n", name);
    return offset + 1;
}

size_t constant_instruction(std::string_view name, const Chunk& chunk, size_t offset) {
    if (offset + 2 >= chunk.code.size()) {
        std::cout << std::format("{:<16} (truncated)\n", name);
        return chunk.code.size();
    }
    uint16_t idx = static_cast<uint16_t>((chunk.code[offset + 1] << 8) | chunk.code[offset + 2]);
    std::cout << std::format("{:<16} {:4d} '", name, idx);
    if (idx < chunk.constants.size()) {
        std::cout << chunk.constants[idx].to_string();
    } else {
        std::cout << "<bad const>";
    }
    std::cout << "'\n";
    return offset + 3;
}

size_t byte_instruction(std::string_view name, const Chunk& chunk, size_t offset) {
    if (offset + 1 >= chunk.code.size()) {
        std::cout << std::format("{:<16} (truncated)\n", name);
        return chunk.code.size();
    }
    uint8_t slot = chunk.code[offset + 1];
    std::cout << std::format("{:<16} {:4d}\n", name, slot);
    return offset + 2;
}

size_t jump_instruction(std::string_view name, int sign, const Chunk& chunk, size_t offset) {
    if (offset + 2 >= chunk.code.size()) {
        std::cout << std::format("{:<16} (truncated)\n", name);
        return chunk.code.size();
    }
    uint16_t jump = static_cast<uint16_t>((chunk.code[offset + 1] << 8) | chunk.code[offset + 2]);
    size_t target = offset + 3 + static_cast<size_t>(sign * jump);
    std::cout << std::format("{:<16} {:4d} -> {:04d}\n", name, offset, target);
    return offset + 3;
}

} // namespace

void Disassembler::disassemble(const Chunk& chunk, std::string_view name) {
    std::cout << std::format("=== {} ===\n", name);
    for (size_t offset = 0; offset < chunk.code.size();) {
        offset = disassemble_instruction(chunk, offset);
    }
}

size_t Disassembler::disassemble_instruction(const Chunk& chunk, size_t offset) {
    std::cout << std::format("{:04d} ", offset);
    if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1]) {
        std::cout << "   | ";
    } else {
        std::cout << std::format("{:4d} ", chunk.lines[offset]);
    }

    auto op = static_cast<Opcode>(chunk.code[offset]);
    switch (op) {
        case Opcode::Constant:     return constant_instruction("OP_CONSTANT", chunk, offset);
        case Opcode::Null:         return simple_instruction("OP_NULL", offset);
        case Opcode::True:         return simple_instruction("OP_TRUE", offset);
        case Opcode::False:        return simple_instruction("OP_FALSE", offset);
        case Opcode::Pop:          return simple_instruction("OP_POP", offset);
        case Opcode::Dup:          return simple_instruction("OP_DUP", offset);
        case Opcode::GetGlobal:    return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case Opcode::SetGlobal:    return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case Opcode::GetLocal:     return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case Opcode::SetLocal:     return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case Opcode::Equal:        return simple_instruction("OP_EQUAL", offset);
        case Opcode::NotEqual:     return simple_instruction("OP_NOT_EQUAL", offset);
        case Opcode::Greater:      return simple_instruction("OP_GREATER", offset);
        case Opcode::GreaterEqual: return simple_instruction("OP_GREATER_EQUAL", offset);
        case Opcode::Less:         return simple_instruction("OP_LESS", offset);
        case Opcode::LessEqual:    return simple_instruction("OP_LESS_EQUAL", offset);
        case Opcode::Add:          return simple_instruction("OP_ADD", offset);
        case Opcode::Sub:          return simple_instruction("OP_SUB", offset);
        case Opcode::Mul:          return simple_instruction("OP_MUL", offset);
        case Opcode::Div:          return simple_instruction("OP_DIV", offset);
        case Opcode::Mod:          return simple_instruction("OP_MOD", offset);
        case Opcode::Exp:          return simple_instruction("OP_EXP", offset);
        case Opcode::Negate:       return simple_instruction("OP_NEGATE", offset);
        case Opcode::Not:          return simple_instruction("OP_NOT", offset);
        case Opcode::BitAnd:       return simple_instruction("OP_BIT_AND", offset);
        case Opcode::BitOr:        return simple_instruction("OP_BIT_OR", offset);
        case Opcode::BitXor:       return simple_instruction("OP_BIT_XOR", offset);
        case Opcode::Jump:         return jump_instruction("OP_JUMP", 1, chunk, offset);
        case Opcode::JumpIfFalse:  return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case Opcode::Loop:         return jump_instruction("OP_LOOP", -1, chunk, offset);
        case Opcode::Call:         return byte_instruction("OP_CALL", chunk, offset);
        case Opcode::Return:       return simple_instruction("OP_RETURN", offset);
        case Opcode::DefineFunc:   return constant_instruction("OP_DEFINE_FUNC", chunk, offset);
        case Opcode::Print:        return simple_instruction("OP_PRINT", offset);
        case Opcode::Println:      return simple_instruction("OP_PRINTLN", offset);
        case Opcode::Halt:         return simple_instruction("OP_HALT", offset);
    }
    std::cout << std::format("Unknown opcode {:02x}\n", chunk.code[offset]);
    return offset + 1;
}

} // namespace luna::compiler
