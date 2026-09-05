#pragma once
// Gravity VM Bytecode disassembler for inspection and debugging.
#include "Chunk.hpp"
#include <string>
#include <string_view>

namespace luna::compiler {

class Disassembler {
public:
    static void disassemble(const Chunk& chunk, std::string_view name);
    static size_t disassemble_instruction(const Chunk& chunk, size_t offset);
};

} // namespace luna::compiler
