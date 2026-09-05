// AST to Bytecode Compiler implementation for Luna.
#include "Compiler.hpp"
#include <format>
#include <stdexcept>

namespace luna::compiler {

namespace {
std::string unescape_string(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                case '\\': out.push_back('\\'); break;
                case '\'': out.push_back('\''); break;
                case '"': out.push_back('"'); break;
                default: out.push_back(s[i]); break;
            }
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}
} // namespace

using namespace luna::ast;

Compiler::Compiler(FunctionType type, std::string name)
    : type_(type), name_(std::move(name)), chunk_(std::make_shared<Chunk>()) {
    LocalVar root;
    root.name = (type_ == FunctionType::Function) ? name_ : "";
    root.depth = 0;
    locals_.push_back(std::move(root));
}

std::shared_ptr<Chunk> Compiler::compile(Stmts* program) {
    if (program) {
        for (const auto& stmt : program->stmts) {
            compile(stmt);
        }
    }
    if (type_ == FunctionType::Script) {
        emit_byte(static_cast<uint8_t>(Opcode::Halt), program ? program->line : 1);
    } else {
        emit_byte(static_cast<uint8_t>(Opcode::Null), 1);
        emit_byte(static_cast<uint8_t>(Opcode::Return), 1);
    }
    return chunk_;
}

void Compiler::emit_byte(uint8_t byte, size_t line) { chunk_->write_byte(byte, line); }
void Compiler::emit_bytes(uint8_t b1, uint8_t b2, size_t line) { emit_byte(b1, line); emit_byte(b2, line); }

uint16_t Compiler::make_constant(vm::Value value, size_t line) {
    size_t idx = chunk_->add_constant(std::move(value));
    return static_cast<uint16_t>(idx);
}

void Compiler::emit_constant(vm::Value value, size_t line) {
    chunk_->emit_constant(std::move(value), line);
}

size_t Compiler::emit_jump(uint8_t instruction, size_t line) {
    emit_byte(instruction, line);
    emit_byte(0xFF, line);
    emit_byte(0xFF, line);
    return chunk_->code.size() - 2;
}

void Compiler::patch_jump(size_t offset) {
    size_t jump = chunk_->code.size() - offset - 2;
    if (jump > UINT16_MAX) throw std::runtime_error("Control flow jump too large.");
    chunk_->code[offset] = static_cast<uint8_t>((jump >> 8) & 0xFF);
    chunk_->code[offset + 1] = static_cast<uint8_t>(jump & 0xFF);
}

void Compiler::emit_loop(size_t loop_start, size_t line) {
    emit_byte(static_cast<uint8_t>(Opcode::Loop), line);
    size_t jump = chunk_->code.size() - loop_start + 2;
    if (jump > UINT16_MAX) throw std::runtime_error("Loop body too large.");
    emit_byte(static_cast<uint8_t>((jump >> 8) & 0xFF), line);
    emit_byte(static_cast<uint8_t>(jump & 0xFF), line);
}

void Compiler::begin_scope() { scope_depth_++; }

void Compiler::end_scope(size_t line) {
    scope_depth_--;
    while (!locals_.empty() && locals_.back().depth > scope_depth_) {
        emit_byte(static_cast<uint8_t>(Opcode::Pop), line);
        locals_.pop_back();
    }
}

void Compiler::declare_variable(std::string_view name) {
    LocalVar local;
    local.name = std::string(name);
    local.depth = scope_depth_;
    locals_.push_back(std::move(local));
}

void Compiler::compile(const Expr& expr) {
    std::visit(Overloaded{
        [this](IntegerLit* i) { emit_constant(vm::Value(static_cast<double>(i->value)), i->line); },
        [this](FloatLit* f) { emit_constant(vm::Value(f->value), f->line); },
        [this](BoolLit* b) { emit_byte(static_cast<uint8_t>(b->value ? Opcode::True : Opcode::False), b->line); },
        [this](NullLit* n) { emit_byte(static_cast<uint8_t>(Opcode::Null), n->line); },
        [this](StringLit* s) { emit_constant(vm::Value(unescape_string(s->value)), s->line); },
        [this](Identifier* id) {
            int slot = -1;
            for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
                if (locals_[i].name == id->name) { slot = i; break; }
            }
            if (slot != -1) {
                emit_bytes(static_cast<uint8_t>(Opcode::GetLocal), static_cast<uint8_t>(slot), id->line);
            } else {
                uint16_t name_idx = make_constant(vm::Value(std::string(id->name)), id->line);
                emit_byte(static_cast<uint8_t>(Opcode::GetGlobal), id->line);
                emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), id->line);
                emit_byte(static_cast<uint8_t>(name_idx & 0xFF), id->line);
            }
        },
        [this](Grouping* g) { compile(g->inner); },
        [this](UnOp* u) {
            compile(u->operand);
            if (u->op.type == lexer::TokenType::Minus) emit_byte(static_cast<uint8_t>(Opcode::Negate), u->line);
            else if (u->op.type == lexer::TokenType::Tilde) emit_byte(static_cast<uint8_t>(Opcode::Not), u->line);
        },
        [this](BinOp* b) {
            compile(b->left);
            compile(b->right);
            switch (b->op.type) {
                case lexer::TokenType::Plus:  emit_byte(static_cast<uint8_t>(Opcode::Add), b->line); break;
                case lexer::TokenType::Minus: emit_byte(static_cast<uint8_t>(Opcode::Sub), b->line); break;
                case lexer::TokenType::Star:  emit_byte(static_cast<uint8_t>(Opcode::Mul), b->line); break;
                case lexer::TokenType::Slash: emit_byte(static_cast<uint8_t>(Opcode::Div), b->line); break;
                case lexer::TokenType::Mod:   emit_byte(static_cast<uint8_t>(Opcode::Mod), b->line); break;
                case lexer::TokenType::Caret: emit_byte(static_cast<uint8_t>(Opcode::Exp), b->line); break;
                case lexer::TokenType::EqEq:  emit_byte(static_cast<uint8_t>(Opcode::Equal), b->line); break;
                case lexer::TokenType::Ne:    emit_byte(static_cast<uint8_t>(Opcode::NotEqual), b->line); break;
                case lexer::TokenType::Lt:    emit_byte(static_cast<uint8_t>(Opcode::Less), b->line); break;
                case lexer::TokenType::Le:    emit_byte(static_cast<uint8_t>(Opcode::LessEqual), b->line); break;
                case lexer::TokenType::Gt:    emit_byte(static_cast<uint8_t>(Opcode::Greater), b->line); break;
                case lexer::TokenType::Ge:    emit_byte(static_cast<uint8_t>(Opcode::GreaterEqual), b->line); break;
                default: break;
            }
        },
        [this](LogicalOp* l) {
            compile(l->left);
            if (l->op.type == lexer::TokenType::And) {
                emit_byte(static_cast<uint8_t>(Opcode::Dup), l->line);
                size_t end_jump = emit_jump(static_cast<uint8_t>(Opcode::JumpIfFalse), l->line);
                emit_byte(static_cast<uint8_t>(Opcode::Pop), l->line);
                compile(l->right);
                patch_jump(end_jump);
            } else {
                emit_byte(static_cast<uint8_t>(Opcode::Dup), l->line);
                emit_byte(static_cast<uint8_t>(Opcode::Not), l->line);
                size_t end_jump = emit_jump(static_cast<uint8_t>(Opcode::JumpIfFalse), l->line);
                emit_byte(static_cast<uint8_t>(Opcode::Pop), l->line);
                compile(l->right);
                patch_jump(end_jump);
            }
        },
        [this](FuncCall* fc) {
            uint16_t name_idx = make_constant(vm::Value(std::string(fc->name)), fc->line);
            emit_byte(static_cast<uint8_t>(Opcode::GetGlobal), fc->line);
            emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), fc->line);
            emit_byte(static_cast<uint8_t>(name_idx & 0xFF), fc->line);

            for (const auto& arg : fc->args) compile(arg);
            emit_bytes(static_cast<uint8_t>(Opcode::Call), static_cast<uint8_t>(fc->args.size()), fc->line);
        }
    }, expr);
}

void Compiler::compile(const Stmt& stmt) {
    std::visit(Overloaded{
        [this](PrintStmt* p) {
            compile(p->value);
            emit_byte(static_cast<uint8_t>(p->newline ? Opcode::Println : Opcode::Print), p->line);
        },
        [this](IfStmt* s) {
            compile(s->test);
            size_t then_jump = emit_jump(static_cast<uint8_t>(Opcode::JumpIfFalse), s->line);
            
            begin_scope();
            if (s->then_body) {
                for (const auto& st : s->then_body->stmts) compile(st);
            }
            end_scope(s->line);

            if (s->else_body) {
                size_t else_jump = emit_jump(static_cast<uint8_t>(Opcode::Jump), s->line);
                patch_jump(then_jump);
                begin_scope();
                for (const auto& st : s->else_body->stmts) compile(st);
                end_scope(s->line);
                patch_jump(else_jump);
            } else {
                patch_jump(then_jump);
            }
        },
        [this](WhileStmt* w) {
            size_t loop_start = chunk_->code.size();
            compile(w->test);
            size_t exit_jump = emit_jump(static_cast<uint8_t>(Opcode::JumpIfFalse), w->line);

            begin_scope();
            if (w->body) {
                for (const auto& st : w->body->stmts) compile(st);
            }
            end_scope(w->line);
            
            emit_loop(loop_start, w->line);
            patch_jump(exit_jump);
        },
        [this](ForStmt* f) {
            compile(f->start);
            uint16_t name_idx = 0;
            std::string ident_name = f->ident ? std::string(f->ident->name) : "i";
            
            if (type_ == FunctionType::Function) {
                declare_variable(ident_name);
            } else {
                name_idx = make_constant(vm::Value(ident_name), f->line);
                emit_byte(static_cast<uint8_t>(Opcode::SetGlobal), f->line);
                emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(name_idx & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(Opcode::Pop), f->line);
            }

            size_t loop_start = chunk_->code.size();

            // Condition
            if (type_ == FunctionType::Function) {
                int var_slot = -1;
                for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
                    if (locals_[i].name == ident_name) { var_slot = i; break; }
                }
                emit_bytes(static_cast<uint8_t>(Opcode::GetLocal), static_cast<uint8_t>(var_slot), f->line);
            } else {
                emit_byte(static_cast<uint8_t>(Opcode::GetGlobal), f->line);
                emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(name_idx & 0xFF), f->line);
            }
            compile(f->end);

            bool negative_step = false;
            if (f->step) {
                if (std::holds_alternative<UnOp*>(*f->step)) {
                    if (std::get<UnOp*>(*f->step)->op.type == lexer::TokenType::Minus) negative_step = true;
                }
            }
            emit_byte(static_cast<uint8_t>(negative_step ? Opcode::GreaterEqual : Opcode::LessEqual), f->line);

            size_t exit_jump = emit_jump(static_cast<uint8_t>(Opcode::JumpIfFalse), f->line);

            if (f->body) {
                for (const auto& st : f->body->stmts) compile(st);
            }

            // Increment
            if (type_ == FunctionType::Function) {
                int var_slot = -1;
                for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
                    if (locals_[i].name == ident_name) { var_slot = i; break; }
                }
                emit_bytes(static_cast<uint8_t>(Opcode::GetLocal), static_cast<uint8_t>(var_slot), f->line);
            } else {
                emit_byte(static_cast<uint8_t>(Opcode::GetGlobal), f->line);
                emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(name_idx & 0xFF), f->line);
            }

            if (f->step) compile(*f->step);
            else emit_constant(vm::Value(1.0), f->line);
            emit_byte(static_cast<uint8_t>(Opcode::Add), f->line);

            if (type_ == FunctionType::Function) {
                int var_slot = -1;
                for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
                    if (locals_[i].name == ident_name) { var_slot = i; break; }
                }
                emit_bytes(static_cast<uint8_t>(Opcode::SetLocal), static_cast<uint8_t>(var_slot), f->line);
                emit_byte(static_cast<uint8_t>(Opcode::Pop), f->line);
            } else {
                emit_byte(static_cast<uint8_t>(Opcode::SetGlobal), f->line);
                emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(name_idx & 0xFF), f->line);
                emit_byte(static_cast<uint8_t>(Opcode::Pop), f->line);
            }

            emit_loop(loop_start, f->line);
            patch_jump(exit_jump);
        },
        [this](Assignment* a) {
            compile(a->right);
            if (std::holds_alternative<Identifier*>(a->left)) {
                auto* id = std::get<Identifier*>(a->left);
                int slot = -1;
                for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
                    if (locals_[i].name == id->name) { slot = i; break; }
                }
                if (slot != -1) {
                    emit_bytes(static_cast<uint8_t>(Opcode::SetLocal), static_cast<uint8_t>(slot), a->line);
                    emit_byte(static_cast<uint8_t>(Opcode::Pop), a->line);
                } else if (type_ == FunctionType::Function) {
                    declare_variable(id->name);
                } else {
                    uint16_t name_idx = make_constant(vm::Value(std::string(id->name)), a->line);
                    emit_byte(static_cast<uint8_t>(Opcode::SetGlobal), a->line);
                    emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), a->line);
                    emit_byte(static_cast<uint8_t>(name_idx & 0xFF), a->line);
                    emit_byte(static_cast<uint8_t>(Opcode::Pop), a->line);
                }
            }
        },
        [this](RetStmt* r) {
            if (std::holds_alternative<NullLit*>(r->value)) {
                emit_byte(static_cast<uint8_t>(Opcode::Null), r->line);
            } else {
                compile(r->value);
            }
            emit_byte(static_cast<uint8_t>(Opcode::Return), r->line);
        },
        [this](FuncCallStmt* fcs) {
            compile(Expr(fcs->call));
            emit_byte(static_cast<uint8_t>(Opcode::Pop), fcs->line);
        },
        [this](FuncDecl* fd) {
            Compiler fn_compiler(FunctionType::Function, std::string(fd->name));
            for (auto* p : fd->params) fn_compiler.declare_variable(p->name);
            
            if (fd->body) {
                for (const auto& st : fd->body->stmts) fn_compiler.compile(st);
            }
            fn_compiler.emit_byte(static_cast<uint8_t>(Opcode::Null), fd->line);
            fn_compiler.emit_byte(static_cast<uint8_t>(Opcode::Return), fd->line);

            auto fn_obj = std::make_shared<vm::ObjFunction>(std::string(fd->name), fd->params.size(), fn_compiler.chunk_);
            uint16_t name_idx = make_constant(vm::Value(std::string(fd->name)), fd->line);
            uint16_t fn_idx = make_constant(vm::Value(fn_obj), fd->line);

            emit_byte(static_cast<uint8_t>(Opcode::DefineFunc), fd->line);
            emit_byte(static_cast<uint8_t>((name_idx >> 8) & 0xFF), fd->line);
            emit_byte(static_cast<uint8_t>(name_idx & 0xFF), fd->line);
            emit_byte(static_cast<uint8_t>((fn_idx >> 8) & 0xFF), fd->line);
            emit_byte(static_cast<uint8_t>(fn_idx & 0xFF), fd->line);
        }
    }, stmt);
}

} // namespace luna::compiler
