#pragma once
// Charcoal1 AST: strongly-typed nodes for statements and expressions.

#include <memory>
#include <string>
#include <vector>

namespace charcoal1 {

struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

// --- expressions -----------------------------------------------------------
struct Expr {
    virtual ~Expr() = default;
    virtual std::string debug() const = 0;
};

struct Literal : Expr {
    double number = 0;
    std::string str;
    bool is_string = false;
    std::string debug() const override {
        return is_string ? ("\"" + str + "\"") : std::to_string(number);
    }
};

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
    std::string debug() const override { return name; }
};

struct BinaryExpr : Expr {
    std::string op; // + - * / == != < > <= >= && ||
    ExprPtr left, right;
    std::string debug() const override {
        return "(" + left->debug() + " " + op + " " + right->debug() + ")";
    }
};

struct CallExpr : Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    std::string debug() const override {
        std::string s = callee + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i) s += ", ";
            s += args[i]->debug();
        }
        return s + ")";
    }
};

// --- statements ------------------------------------------------------------
struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string debug(int depth = 0) const = 0;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {}
    std::string debug(int depth = 0) const override {
        return std::string(depth * 2, ' ') + expr->debug() + ";";
    }
};

struct VarDecl : Stmt {
    std::string name;
    ExprPtr init; // may be null
    std::string debug(int depth = 0) const override {
        return std::string(depth * 2, ' ') + "let " + name +
               (init ? (" = " + init->debug()) : "") + ";";
    }
};

struct FuncDecl : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    std::string debug(int depth = 0) const override {
        std::string s(depth * 2, ' ');
        s += "function " + name + "(";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i) s += ", ";
            s += params[i];
        }
        s += ") {\n";
        for (auto& st : body) s += st->debug(depth + 1) + "\n";
        return s + std::string(depth * 2, ' ') + "}";
    }
};

struct ReturnStmt : Stmt {
    ExprPtr value; // may be null
    std::string debug(int depth = 0) const override {
        return std::string(depth * 2, ' ') + "return" +
               (value ? (" " + value->debug()) : "") + ";";
    }
};

struct IfStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> then_body, else_body;
    std::string debug(int depth = 0) const override {
        std::string s(depth * 2, ' ');
        s += "if (" + cond->debug() + ") {\n";
        for (auto& st : then_body) s += st->debug(depth + 1) + "\n";
        s += std::string(depth * 2, ' ') + "}";
        if (!else_body.empty()) {
            s += " else {\n";
            for (auto& st : else_body) s += st->debug(depth + 1) + "\n";
            s += std::string(depth * 2, ' ') + "}";
        }
        return s;
    }
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::vector<StmtPtr> body;
    std::string debug(int depth = 0) const override {
        std::string s(depth * 2, ' ');
        s += "while (" + cond->debug() + ") {\n";
        for (auto& st : body) s += st->debug(depth + 1) + "\n";
        return s + std::string(depth * 2, ' ') + "}";
    }
};

} // namespace charcoal1
