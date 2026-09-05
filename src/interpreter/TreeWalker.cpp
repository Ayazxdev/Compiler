// Reference Tree-Walking Interpreter implementation for Luna.
#include "TreeWalker.hpp"
#include <cmath>
#include <format>
#include <iostream>
#include <stdexcept>

namespace luna::interpreter {

using namespace luna::ast;

TreeWalker::TreeWalker() : env_(std::make_shared<Environment>()) {
    output_handler_ = [](std::string_view s) {
        std::cout << s;
    };
}

void TreeWalker::interpret(Stmts* program) {
    if (!program) return;
    try {
        for (const auto& stmt : program->stmts) {
            execute(stmt);
            if (is_returning_) break;
        }
    } catch (const std::exception& e) {
        output_handler_(std::format("Runtime error: {}\n", e.what()));
    }
}

vm::Value TreeWalker::evaluate(const Expr& expr) {
    return std::visit(Overloaded{
        [](IntegerLit* i) -> vm::Value { return vm::Value(static_cast<double>(i->value)); },
        [](FloatLit* f) -> vm::Value { return vm::Value(f->value); },
        [](BoolLit* b) -> vm::Value { return vm::Value(b->value); },
        [](NullLit*) -> vm::Value { return vm::Value(); },
        [](StringLit* s) -> vm::Value { return vm::Value(std::string(s->value)); },
        [this](Identifier* id) -> vm::Value {
            if (vm::Value* val = env_->get(id->name)) return *val;
            throw std::runtime_error(std::format("Undefined variable '{}'.", id->name));
        },
        [this](Grouping* g) -> vm::Value { return evaluate(g->inner); },
        [this](UnOp* u) -> vm::Value {
            vm::Value right = evaluate(u->operand);
            if (u->op.type == lexer::TokenType::Minus) {
                if (!right.is_number()) throw std::runtime_error("Operand must be a number.");
                return vm::Value(-right.as_number());
            } else if (u->op.type == lexer::TokenType::Tilde) {
                return vm::Value(!right.is_truthy());
            }
            return vm::Value();
        },
        [this](BinOp* b) -> vm::Value {
            vm::Value left = evaluate(b->left);
            vm::Value right = evaluate(b->right);
            switch (b->op.type) {
                case lexer::TokenType::Plus:
                    if (left.is_string() && right.is_string()) {
                        return vm::Value(left.as_string() + right.as_string());
                    }
                    if (left.is_number() && right.is_number()) {
                        return vm::Value(left.as_number() + right.as_number());
                    }
                    throw std::runtime_error("Operands must be two numbers or two strings.");
                case lexer::TokenType::Minus:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() - right.as_number());
                case lexer::TokenType::Star:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() * right.as_number());
                case lexer::TokenType::Slash:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() / right.as_number());
                case lexer::TokenType::Mod:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(std::fmod(left.as_number(), right.as_number()));
                case lexer::TokenType::Caret:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(std::pow(left.as_number(), right.as_number()));
                case lexer::TokenType::EqEq: return vm::Value(left == right);
                case lexer::TokenType::Ne:   return vm::Value(!(left == right));
                case lexer::TokenType::Lt:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() < right.as_number());
                case lexer::TokenType::Le:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() <= right.as_number());
                case lexer::TokenType::Gt:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() > right.as_number());
                case lexer::TokenType::Ge:
                    if (!left.is_number() || !right.is_number()) throw std::runtime_error("Operands must be numbers.");
                    return vm::Value(left.as_number() >= right.as_number());
                default: return vm::Value();
            }
        },
        [this](LogicalOp* l) -> vm::Value {
            vm::Value left = evaluate(l->left);
            if (l->op.type == lexer::TokenType::Or) {
                if (left.is_truthy()) return left;
            } else { // And
                if (!left.is_truthy()) return left;
            }
            return evaluate(l->right);
        },
        [this](FuncCall* call) -> vm::Value {
            vm::Value* callee_val = env_->get(call->name);
            if (!callee_val) throw std::runtime_error(std::format("Undefined function '{}'.", call->name));

            // Function call logic relies on custom TreeWalker function objects,
            // but for simplicity in differential testing oracle we assume they are handled or throw.
            throw std::runtime_error("TreeWalker function call execution not fully implemented in variant rewrite.");
        }
    }, expr);
}

void TreeWalker::execute(const Stmt& stmt) {
    std::visit(Overloaded{
        [this](PrintStmt* p) {
            vm::Value val = evaluate(p->value);
            output_handler_(val.to_string());
            if (p->newline) output_handler_("\n");
        },
        [this](IfStmt* s) {
            vm::Value condition = evaluate(s->test);
            if (condition.is_truthy()) {
                if (s->then_body) {
                    auto previous = env_;
                    env_ = env_->new_child();
                    for (const auto& st : s->then_body->stmts) {
                        execute(st);
                        if (is_returning_) break;
                    }
                    env_ = previous;
                }
            } else {
                if (s->else_body) {
                    auto previous = env_;
                    env_ = env_->new_child();
                    for (const auto& st : s->else_body->stmts) {
                        execute(st);
                        if (is_returning_) break;
                    }
                    env_ = previous;
                }
            }
        },
        [this](WhileStmt* w) {
            while (evaluate(w->test).is_truthy()) {
                if (w->body) {
                    auto previous = env_;
                    env_ = env_->new_child();
                    for (const auto& st : w->body->stmts) {
                        execute(st);
                        if (is_returning_) break;
                    }
                    env_ = previous;
                }
                if (is_returning_) break;
            }
        },
        [this](ForStmt* f) {
            auto previous = env_;
            env_ = env_->new_child();
            
            vm::Value start_val = evaluate(f->start);
            std::string id_name = f->ident ? std::string(f->ident->name) : "i";
            env_->set(id_name, start_val);
            
            while (true) {
                vm::Value current_val = *env_->get(id_name);
                vm::Value end_val = evaluate(f->end);
                
                bool is_negative = false;
                if (f->step) {
                    if (std::holds_alternative<UnOp*>(*f->step)) {
                        if (std::get<UnOp*>(*f->step)->op.type == lexer::TokenType::Minus) is_negative = true;
                    }
                }
                
                if (is_negative) {
                    if (current_val.as_number() < end_val.as_number()) break;
                } else {
                    if (current_val.as_number() > end_val.as_number()) break;
                }
                
                if (f->body) {
                    auto inner_prev = env_;
                    env_ = env_->new_child();
                    for (const auto& st : f->body->stmts) {
                        execute(st);
                        if (is_returning_) break;
                    }
                    env_ = inner_prev;
                }
                if (is_returning_) break;
                
                vm::Value step_val = f->step ? evaluate(*f->step) : vm::Value(1.0);
                env_->set(id_name, vm::Value(current_val.as_number() + step_val.as_number()));
            }
            
            env_ = previous;
        },
        [this](Assignment* a) {
            vm::Value val = evaluate(a->right);
            if (std::holds_alternative<Identifier*>(a->left)) {
                env_->set(std::get<Identifier*>(a->left)->name, val);
            }
        },
        [this](FuncDecl* fd) {
            // Placeholder: A full treewalker would capture the environment and AST node
            throw std::runtime_error("TreeWalker function declaration not fully implemented in variant rewrite.");
        },
        [this](RetStmt* r) {
            if (std::holds_alternative<NullLit*>(r->value)) {
                return_value_ = vm::Value();
            } else {
                return_value_ = evaluate(r->value);
            }
            is_returning_ = true;
        },
        [this](FuncCallStmt* fcs) {
            evaluate(Expr(fcs->call));
        }
    }, stmt);
}

} // namespace luna::interpreter
