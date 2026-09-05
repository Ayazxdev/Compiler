#pragma once
// Bytecode chunk holding instructions, constant pool, and debug line numbers for the Gravity VM.
#include "Opcode.hpp"
#include "../vm/Value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace luna::compiler {

struct Chunk {
    std::vector<uint8_t>   code;
    std::vector<vm::Value> constants;
    std::vector<size_t>    lines;

    void write_byte(uint8_t byte, size_t line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    void write_opcode(Opcode op, size_t line) {
        write_byte(static_cast<uint8_t>(op), line);
    }

    void write_uint16(uint16_t val, size_t line) {
        write_byte(static_cast<uint8_t>((val >> 8) & 0xFF), line);
        write_byte(static_cast<uint8_t>(val & 0xFF), line);
    }

    size_t add_constant(vm::Value value) {
        constants.push_back(std::move(value));
        return constants.size() - 1;
    }

    void emit_constant(vm::Value value, size_t line) {
        size_t idx = add_constant(std::move(value));
        write_opcode(Opcode::Constant, line);
        write_uint16(static_cast<uint16_t>(idx), line);
    }

    [[nodiscard]] size_t size() const noexcept { return code.size(); }
    [[nodiscard]] bool empty() const noexcept { return code.empty(); }
};

} // namespace luna::compiler
