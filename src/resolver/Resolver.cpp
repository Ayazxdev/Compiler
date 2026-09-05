// Semantic Analysis and Scope Resolution pass for Luna programs.
#include "Resolver.hpp"
#include <format>

namespace luna::resolver {

using namespace luna::ast;

void Resolver::resolve(Stmts* program) {
    if (!program) return;
    
    // Push implicit global script scope
    scopes_.push_back(FunctionScope{"<script>", ScopeType::Global, {}, 0, 0});
    
    for (const auto& stmt : program->stmts) {
        resolve(stmt);
    }
    
    scopes_.pop_back();
}

void Resolver::begin_scope() {
    scopes_.back().scope_depth++;
}

void Resolver::end_scope() {
    auto& current = scopes_.back();
    current.scope_depth--;
    while (!current.locals.empty() && current.locals.back().depth > current.scope_depth) {
        current.locals.pop_back();
    }
}

void Resolver::declare(std::string_view name, std::size_t line) {
    auto& current = scopes_.back();
    if (current.type == ScopeType::Global && current.scope_depth == 0) return;

    for (auto it = current.locals.rbegin(); it != current.locals.rend(); ++it) {
        if (it->depth < current.scope_depth) break;
        if (it->name == name) {
            throw ResolveError(std::format("Variable '{}' already declared in this scope.", name), line);
        }
    }

    if (current.local_count >= 255) {
        throw ResolveError("Too many local variables in function.", line);
    }

    current.locals.push_back(Local{std::string(name), current.scope_depth, current.local_count++});
}

void Resolver::resolve_local(const Identifier* expr) {
    if (scopes_.empty()) return;

    auto& current = scopes_.back();
    for (auto it = current.locals.rbegin(); it != current.locals.rend(); ++it) {
        if (it->name == expr->name) {
            resolved_locals_[expr] = it->slot;
            return;
        }
    }
}

void Resolver::resolve(const Expr& expr) {
    std::visit(Overloaded{
        [](IntegerLit*) {},
        [](FloatLit*) {},
        [](BoolLit*) {},
        [](NullLit*) {},
        [](StringLit*) {},
        [this](Identifier* id) {
            resolve_local(id);
        },
        [this](Grouping* g) {
            resolve(g->inner);
        },
        [this](UnOp* u) {
            resolve(u->operand);
        },
        [this](BinOp* b) {
            resolve(b->left);
            resolve(b->right);
        },
        [this](LogicalOp* l) {
            resolve(l->left);
            resolve(l->right);
        },
        [this](FuncCall* call) {
            for (const auto& arg : call->args) {
                resolve(arg);
            }
        }
    }, expr);
}

void Resolver::resolve(const Stmt& stmt) {
    std::visit(Overloaded{
        [this](PrintStmt* p) {
            resolve(p->value);
        },
        [this](IfStmt* s) {
            resolve(s->test);
            if (s->then_body) {
                begin_scope();
                for (const auto& st : s->then_body->stmts) resolve(st);
                end_scope();
            }
            if (s->else_body) {
                begin_scope();
                for (const auto& st : s->else_body->stmts) resolve(st);
                end_scope();
            }
        },
        [this](WhileStmt* w) {
            resolve(w->test);
            if (w->body) {
                begin_scope();
                for (const auto& st : w->body->stmts) resolve(st);
                end_scope();
            }
        },
        [this](ForStmt* f) {
            begin_scope();
            declare(f->ident->name, f->ident->line);
            resolve_local(f->ident);
            resolve(f->start);
            resolve(f->end);
            if (f->step) resolve(*f->step);
            if (f->body) {
                for (const auto& st : f->body->stmts) resolve(st);
            }
            end_scope();
        },
        [this](FuncDecl* fd) {
            declare(fd->name, fd->line);
            scopes_.push_back(FunctionScope{std::string(fd->name), ScopeType::Function, {}, 0, 0});
            begin_scope();
            
            // Slot 0 is the function itself (for recursion)
            scopes_.back().locals.push_back(Local{std::string(fd->name), 1, scopes_.back().local_count++});

            for (const auto* param : fd->params) {
                declare(param->name, param->line);
            }

            if (fd->body) {
                for (const auto& st : fd->body->stmts) resolve(st);
            }

            end_scope();
            scopes_.pop_back();
        },
        [this](RetStmt* r) {
            if (scopes_.empty() || scopes_.back().type == ScopeType::Global) {
                throw ResolveError("Cannot return from top-level script code.", r->line);
            }
            resolve(r->value);
        },
        [this](Assignment* a) {
            resolve(a->right);
            if (std::holds_alternative<Identifier*>(a->left)) {
                resolve_local(std::get<Identifier*>(a->left));
            }
        },
        [this](FuncCallStmt* fcs) {
            for (const auto& arg : fcs->call->args) {
                resolve(arg);
            }
        }
    }, stmt);
}

} // namespace luna::resolver
