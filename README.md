# Custom Compiler & Sandboxed Bytecode VM

A high-performance C++26 compiler and sandboxed bytecode Virtual Machine (**Gravity**) designed specifically for executing **Luna**, a custom-designed programming language.

The compiler toolchain compiles source scripts written in the Luna language into bytecode for execution on the Gravity sandboxed VM, featuring zero-copy lexical scanning, top-down operator precedence parsing, bump arena memory allocation, and complete syscall/IO isolation.

---

## Key Features

- **Luna Language Target**: Built dedicated to compiling and running the custom **Luna** programming language (featuring block-scoped definitions, dynamic types, first-class functions, and lexical closures).
- **Gravity Sandboxed Bytecode VM**: Register/stack-based bytecode VM with default-deny opcode architecture and enforceable resource quotas (instruction step budgets, call frame limits, evaluation stack boundaries).
- **Zero-Copy Lexer**: Fast `std::string_view`-based token scanner operating at **245 MiB/s**.
- **Unified Pratt Expression Parser**: Combines recursive descent for statements (`if`, `while`, `for`, `func`, `ret`) with Vaughan Pratt’s top-down operator precedence algorithm for expressions.
- **Thread-Isolated Bump Arena**: Memory allocation model for AST nodes with $O(1)$ allocation and instant teardown.
- **RISC-Style Bytecode ISA**: 25+ opcode instruction set architecture designed for efficient VM dispatch without variable-length decode overhead.
- **Differential Testing Oracle**: Maintained tree-walking AST interpreter verifying bytecode VM execution parity across test suites.

---

## Quickstart & Build Instructions

### Prerequisites
- **CMake** $\ge 3.25$
- **C++20 Compiler**: Clang $\ge 16$, GCC $\ge 13$, or MSVC 2022
- **Ninja** build system

### Build via CMake Presets

```bash
# Clone the repository
cd CD

# Configure & build in Release mode (maximum performance)
cmake --preset release
cmake --build build/release

# Run the full test suite
ctest --test-dir build/release --output-on-failure

# Run the Google Benchmark suite
./build/release/bench/luna_bench
```

### CLI Usage

```bash
# Display version and engine info
./build/release/src/luna --version

# Run a Luna script (.luna)
./build/release/src/luna examples/myscript.luna
./build/release/src/luna examples/website-example.luna
./build/release/src/luna examples/mandel.luna
./build/release/src/luna examples/dragon.luna
```

---

## Language Syntax & Features

```lua
-- Variable assignment
x := 10
pi := 3.141592
name := "Luna"

-- Functions & Recursion
func factorial(n)
    if n <= 1 then
        ret 1
    else
        ret n * factorial(n - 1)
    end
end

-- Loops
sum := 0
for i := 1, 10 do
    sum := sum + i
end

println("Factorial of 5: " + factorial(5))
println("Sum 1..10: " + sum)
```

---