// Gravity Virtual Machine Execution Engine implementation with Sandboxing & Resource Quotas.
#include "VM.hpp"
#include "../compiler/Compiler.hpp"
#include "../compiler/Opcode.hpp"
#include "../memory/Arena.hpp"
#include "../parser/Parser.hpp"
#include "../resolver/Resolver.hpp"
#include <cstdarg>
#include <format>
#include <iostream>

namespace luna::vm {

using compiler::Opcode;

VM::VM() : limits_{} {
    stack_.reserve(256);
    frames_.reserve(64);
}

VM::VM(ResourceLimits limits) : limits_(limits) {
    stack_.reserve(256);
    frames_.reserve(64);
}

void VM::push(Value value) {
    if (stack_.size() >= limits_.max_stack) {
        throw std::runtime_error("Sandbox resource limit exceeded: Stack ceiling overflow.");
    }
    stack_.push_back(std::move(value));
}

Value VM::pop() {
    if (stack_.empty()) {
        throw std::runtime_error("Stack underflow.");
    }
    Value v = std::move(stack_.back());
    stack_.pop_back();
    return v;
}

Value VM::peek(size_t distance) const {
    if (distance >= stack_.size()) {
        throw std::runtime_error("Stack peek out of bounds.");
    }
    return stack_[stack_.size() - 1 - distance];
}

void VM::print_output(std::string_view text) {
    if (output_handler_) {
        output_handler_(text);
    } else {
        std::cout << text;
    }
}

bool VM::call(Value::FunctionPtr function, uint8_t arg_count) {
    if (arg_count != function->arity) {
        throw std::runtime_error(std::format("Expected {} arguments but got {}.", function->arity, arg_count));
    }
    if (frames_.size() >= limits_.max_frames) {
        throw std::runtime_error("Sandbox resource limit exceeded: Maximum call stack recursion depth.");
    }
    CallFrame frame;
    frame.function = function;
    frame.ip = 0;
    frame.slots = stack_.size() - 1 - arg_count;
    frames_.push_back(std::move(frame));
    return true;
}

bool VM::call_value(const Value& callee, uint8_t arg_count) {
    if (callee.is_function()) {
        return call(callee.as_function(), arg_count);
    }
    throw std::runtime_error("Can only call functions.");
}

InterpretResult VM::interpret(std::string_view source) {
    try {
        memory::Arena arena;
        parser::Parser parser(source, arena);
        ast::Stmts* program = parser.parse();

        resolver::Resolver resolver;
        resolver.resolve(program);

        compiler::Compiler compiler;
        std::shared_ptr<compiler::Chunk> chunk = compiler.compile(program);

        return run(std::move(chunk));
    } catch (const lexer::LexError& e) {
        std::cerr << std::format("[Lexer Error at line {}]: {}\n", e.line, e.what());
        return InterpretResult::CompileError;
    } catch (const parser::ParseError& e) {
        std::cerr << std::format("[Parser Error at line {}]: {}\n", e.line, e.what());
        return InterpretResult::CompileError;
    } catch (const std::exception& e) {
        std::cerr << std::format("[Compile Error]: {}\n", e.what());
        return InterpretResult::CompileError;
    }
}

InterpretResult VM::run(std::shared_ptr<compiler::Chunk> chunk) {
    auto main_fn = std::make_shared<ObjFunction>("<main>", 0, chunk);
    stack_.clear();
    frames_.clear();
    instructions_executed_ = 0;

    push(Value(main_fn));
    call(main_fn, 0);

    try {
        while (!frames_.empty()) {
            if (++instructions_executed_ > limits_.max_instructions) {
                throw std::runtime_error("Sandbox resource limit exceeded: Maximum instruction step quota reached.");
            }

            CallFrame* frame = &frames_.back();
            const compiler::Chunk& c = *frame->function->chunk;

            if (frame->ip >= c.code.size()) {
                break;
            }

            auto op = static_cast<Opcode>(c.code[frame->ip++]);
            switch (op) {
                case Opcode::Constant: {
                    uint16_t idx = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    push(c.constants[idx]);
                    break;
                }
                case Opcode::Null: {
                    push(Value());
                    break;
                }
                case Opcode::True: {
                    push(Value(true));
                    break;
                }
                case Opcode::False: {
                    push(Value(false));
                    break;
                }
                case Opcode::Pop: {
                    pop();
                    break;
                }
                case Opcode::Dup: {
                    push(peek(0));
                    break;
                }
                case Opcode::GetGlobal: {
                    uint16_t idx = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    const std::string& name = c.constants[idx].as_string();
                    auto it = globals_.find(name);
                    if (it == globals_.end()) {
                        throw std::runtime_error(std::format("Undefined variable '{}'.", name));
                    }
                    push(it->second);
                    break;
                }
                case Opcode::SetGlobal: {
                    uint16_t idx = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    const std::string& name = c.constants[idx].as_string();
                    globals_[name] = peek(0);
                    break;
                }
                case Opcode::GetLocal: {
                    uint8_t slot = c.code[frame->ip++];
                    push(stack_[frame->slots + slot]);
                    break;
                }
                case Opcode::SetLocal: {
                    uint8_t slot = c.code[frame->ip++];
                    stack_[frame->slots + slot] = peek(0);
                    break;
                }
                case Opcode::Equal: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a == b));
                    break;
                }
                case Opcode::NotEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a != b));
                    break;
                }
                case Opcode::Greater: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a > b));
                    break;
                }
                case Opcode::GreaterEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a >= b));
                    break;
                }
                case Opcode::Less: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a < b));
                    break;
                }
                case Opcode::LessEqual: {
                    Value b = pop();
                    Value a = pop();
                    push(Value(a <= b));
                    break;
                }
                case Opcode::Add: {
                    Value b = pop();
                    Value a = pop();
                    push(a + b);
                    break;
                }
                case Opcode::Sub: {
                    Value b = pop();
                    Value a = pop();
                    push(a - b);
                    break;
                }
                case Opcode::Mul: {
                    Value b = pop();
                    Value a = pop();
                    push(a * b);
                    break;
                }
                case Opcode::Div: {
                    Value b = pop();
                    Value a = pop();
                    push(a / b);
                    break;
                }
                case Opcode::Mod: {
                    Value b = pop();
                    Value a = pop();
                    push(a % b);
                    break;
                }
                case Opcode::Exp: {
                    Value b = pop();
                    Value a = pop();
                    push(a.pow(b));
                    break;
                }
                case Opcode::Negate: {
                    Value a = pop();
                    push(-a);
                    break;
                }
                case Opcode::Not: {
                    Value a = pop();
                    push(Value(!a.is_truthy()));
                    break;
                }
                case Opcode::BitAnd: {
                    Value b = pop();
                    Value a = pop();
                    long long ia = static_cast<long long>(a.as_number());
                    long long ib = static_cast<long long>(b.as_number());
                    push(Value(static_cast<double>(ia & ib)));
                    break;
                }
                case Opcode::BitOr: {
                    Value b = pop();
                    Value a = pop();
                    long long ia = static_cast<long long>(a.as_number());
                    long long ib = static_cast<long long>(b.as_number());
                    push(Value(static_cast<double>(ia | ib)));
                    break;
                }
                case Opcode::BitXor: {
                    Value b = pop();
                    Value a = pop();
                    long long ia = static_cast<long long>(a.as_number());
                    long long ib = static_cast<long long>(b.as_number());
                    push(Value(static_cast<double>(ia ^ ib)));
                    break;
                }
                case Opcode::Jump: {
                    uint16_t offset = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2 + offset;
                    break;
                }
                case Opcode::JumpIfFalse: {
                    uint16_t offset = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    Value cond = pop();
                    if (!cond.is_truthy()) {
                        frame->ip += offset;
                    }
                    break;
                }
                case Opcode::Loop: {
                    uint16_t offset = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    frame->ip -= offset;
                    break;
                }
                case Opcode::Call: {
                    uint8_t argc = c.code[frame->ip++];
                    Value callee = peek(argc);
                    call_value(callee, argc);
                    break;
                }
                case Opcode::Return: {
                    Value result = pop();
                    size_t frame_slots = frame->slots;
                    frames_.pop_back();
                    if (frames_.empty()) {
                        return InterpretResult::Ok;
                    }
                    // Restore stack to before call
                    while (stack_.size() > frame_slots) {
                        stack_.pop_back();
                    }
                    push(std::move(result));
                    break;
                }
                case Opcode::DefineFunc: {
                    uint16_t name_idx = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    uint16_t fn_idx = static_cast<uint16_t>((c.code[frame->ip] << 8) | c.code[frame->ip + 1]);
                    frame->ip += 2;
                    const std::string& name = c.constants[name_idx].as_string();
                    globals_[name] = c.constants[fn_idx];
                    break;
                }
                case Opcode::Print: {
                    Value v = pop();
                    print_output(v.stringify());
                    break;
                }
                case Opcode::Println: {
                    Value v = pop();
                    print_output(v.stringify() + "\n");
                    break;
                }
                case Opcode::Halt: {
                    return InterpretResult::Ok;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << std::format("[Runtime Error]: {}\n", e.what());
        return InterpretResult::RuntimeError;
    }

    return InterpretResult::Ok;
}

} // namespace luna::vm
