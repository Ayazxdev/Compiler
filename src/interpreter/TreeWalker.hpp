#pragma once
// Reference Tree-Walking Interpreter for the Luna language (Differential Testing Oracle).
#include "../ast/AST.hpp"
#include "../vm/Value.hpp"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace luna::interpreter {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr)
        : parent_(std::move(parent)) {}

    [[nodiscard]] vm::Value* get(std::string_view name) {
        for (auto curr = this; curr != nullptr; curr = curr->parent_.get()) {
            auto it = curr->vars_.find(std::string(name));
            if (it != curr->vars_.end()) {
                return &it->second;
            }
        }
        return nullptr;
    }

    void set(std::string_view name, vm::Value val) {
        for (auto curr = this; curr != nullptr; curr = curr->parent_.get()) {
            auto it = curr->vars_.find(std::string(name));
            if (it != curr->vars_.end()) {
                it->second = std::move(val);
                return;
            }
        }
        vars_[std::string(name)] = std::move(val);
    }

    [[nodiscard]] std::shared_ptr<Environment> new_child() {
        return std::make_shared<Environment>(shared_from_this());
    }

private:
    std::shared_ptr<Environment>            parent_;
    std::unordered_map<std::string, vm::Value> vars_;
};

class TreeWalker {
public:
    TreeWalker();
    ~TreeWalker() = default;

    void interpret(ast::Stmts* program);
    vm::Value evaluate(const ast::Expr& expr);
    void execute(const ast::Stmt& stmt);

    void set_output_handler(std::function<void(std::string_view)> handler) {
        output_handler_ = std::move(handler);
    }

private:
    std::shared_ptr<Environment> env_;
    std::function<void(std::string_view)> output_handler_;
    
    // Internal mechanism to pass values around during statement execution if needed
    vm::Value return_value_;
    bool is_returning_{false};
};

} // namespace luna::interpreter
