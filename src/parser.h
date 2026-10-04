#pragma once
// Charcoal1 recursive-descent parser: tokens -> AST.

#include <stdexcept>
#include <string>
#include <vector>

#include "ast.h"
#include "lexer.h"

namespace charcoal1 {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parses the full program into a statement list.
    std::vector<StmtPtr> parse_program();

private:
    const Token& peek() const;
    const Token& advance();
    bool check(TokenKind kind, const std::string& text = "") const;
    bool match(TokenKind kind, const std::string& text = "");
    void expect(TokenKind kind, const std::string& text);

    StmtPtr parse_statement();
    StmtPtr parse_var_decl();
    StmtPtr parse_func_decl();
    StmtPtr parse_return();
    StmtPtr parse_if();
    StmtPtr parse_while();
    StmtPtr parse_expr_stmt();
    std::vector<StmtPtr> parse_block();

    ExprPtr parse_expr();       // assignment (lowest)
    ExprPtr parse_equality();   // == !=
    ExprPtr parse_comparison(); // < > <= >=
    ExprPtr parse_additive();   // + -
    ExprPtr parse_multiplicative(); // * /
    ExprPtr parse_primary();

    std::vector<Token> toks_;
    size_t pos_ = 0;
};

} // namespace charcoal1
