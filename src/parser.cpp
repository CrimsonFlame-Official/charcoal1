// Charcoal1 parser implementation.
#include "parser.h"

namespace charcoal1 {

Parser::Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

const Token& Parser::peek() const { return toks_[pos_]; }
const Token& Parser::advance() { return toks_[pos_++]; }

bool Parser::check(TokenKind kind, const std::string& text) const {
    if (peek().kind != kind) return false;
    return text.empty() || peek().text == text;
}

bool Parser::match(TokenKind kind, const std::string& text) {
    if (!check(kind, text)) return false;
    advance();
    return true;
}

void Parser::expect(TokenKind kind, const std::string& text) {
    if (!match(kind, text))
        throw ParseError("expected '" + text + "' but found '" + peek().text + "'");
}

std::vector<StmtPtr> Parser::parse_program() {
    std::vector<StmtPtr> out;
    while (!check(TokenKind::Eof)) out.push_back(parse_statement());
    return out;
}

StmtPtr Parser::parse_statement() {
    if (check(TokenKind::Keyword, "let") || check(TokenKind::Keyword, "const") ||
        check(TokenKind::Keyword, "var"))
        return parse_var_decl();
    if (check(TokenKind::Keyword, "function")) return parse_func_decl();
    if (check(TokenKind::Keyword, "return")) return parse_return();
    if (check(TokenKind::Keyword, "if")) return parse_if();
    if (check(TokenKind::Keyword, "while")) return parse_while();
    return parse_expr_stmt();
}

StmtPtr Parser::parse_var_decl() {
    advance(); // let/const/var
    if (!check(TokenKind::Identifier))
        throw ParseError("expected identifier after declaration");
    std::string name = advance().text;
    ExprPtr init;
    if (match(TokenKind::Operator, "=")) init = parse_expr();
    match(TokenKind::Punct, ";");
    auto vd = std::make_shared<VarDecl>();
    vd->name = name;
    vd->init = init;
    return vd;
}

StmtPtr Parser::parse_func_decl() {
    expect(TokenKind::Keyword, "function");
    if (!check(TokenKind::Identifier)) throw ParseError("expected function name");
    std::string name = advance().text;
    expect(TokenKind::Punct, "(");
    std::vector<std::string> params;
    if (!check(TokenKind::Punct, ")")) {
        do {
            if (!check(TokenKind::Identifier)) throw ParseError("expected parameter name");
            params.push_back(advance().text);
        } while (match(TokenKind::Punct, ","));
    }
    expect(TokenKind::Punct, ")");
    auto fd = std::make_shared<FuncDecl>();
    fd->name = name;
    fd->params = std::move(params);
    fd->body = parse_block();
    return fd;
}

std::vector<StmtPtr> Parser::parse_block() {
    expect(TokenKind::Punct, "{");
    std::vector<StmtPtr> body;
    while (!check(TokenKind::Punct, "}") && !check(TokenKind::Eof))
        body.push_back(parse_statement());
    expect(TokenKind::Punct, "}");
    return body;
}

StmtPtr Parser::parse_return() {
    expect(TokenKind::Keyword, "return");
    auto rs = std::make_shared<ReturnStmt>();
    if (!check(TokenKind::Punct, ";") && !check(TokenKind::Punct, "}") &&
        !check(TokenKind::Eof))
        rs->value = parse_expr();
    match(TokenKind::Punct, ";");
    return rs;
}

StmtPtr Parser::parse_if() {
    expect(TokenKind::Keyword, "if");
    expect(TokenKind::Punct, "(");
    ExprPtr cond = parse_expr();
    expect(TokenKind::Punct, ")");
    auto is = std::make_shared<IfStmt>();
    is->cond = cond;
    is->then_body = parse_block();
    if (match(TokenKind::Keyword, "else")) is->else_body = parse_block();
    return is;
}

StmtPtr Parser::parse_while() {
    expect(TokenKind::Keyword, "while");
    expect(TokenKind::Punct, "(");
    ExprPtr cond = parse_expr();
    expect(TokenKind::Punct, ")");
    auto ws = std::make_shared<WhileStmt>();
    ws->cond = cond;
    ws->body = parse_block();
    return ws;
}

StmtPtr Parser::parse_expr_stmt() {
    ExprPtr e = parse_expr();
    match(TokenKind::Punct, ";");
    return std::make_shared<ExprStmt>(e);
}

ExprPtr Parser::parse_expr() {
    // assignment: identifier = expr  (right-associative)
    ExprPtr e = parse_equality();
    if (match(TokenKind::Operator, "=")) {
        auto* id = dynamic_cast<Identifier*>(e.get());
        if (!id) throw ParseError("invalid assignment target");
        auto b = std::make_shared<BinaryExpr>();
        b->op = "=";
        b->left = std::make_shared<Identifier>(id->name);
        b->right = parse_expr();
        return b;
    }
    return e;
}

ExprPtr Parser::parse_equality() {
    ExprPtr e = parse_comparison();
    while (check(TokenKind::Operator, "==") || check(TokenKind::Operator, "!=")) {
        std::string op = advance().text;
        auto b = std::make_shared<BinaryExpr>();
        b->op = op;
        b->left = e;
        b->right = parse_comparison();
        e = b;
    }
    return e;
}

ExprPtr Parser::parse_comparison() {
    ExprPtr e = parse_additive();
    while (check(TokenKind::Operator, "<") || check(TokenKind::Operator, ">") ||
           check(TokenKind::Operator, "<=") || check(TokenKind::Operator, ">=")) {
        std::string op = advance().text;
        auto b = std::make_shared<BinaryExpr>();
        b->op = op;
        b->left = e;
        b->right = parse_additive();
        e = b;
    }
    return e;
}

ExprPtr Parser::parse_additive() {
    ExprPtr e = parse_multiplicative();
    while (check(TokenKind::Operator, "+") || check(TokenKind::Operator, "-")) {
        std::string op = advance().text;
        auto b = std::make_shared<BinaryExpr>();
        b->op = op;
        b->left = e;
        b->right = parse_multiplicative();
        e = b;
    }
    return e;
}

ExprPtr Parser::parse_multiplicative() {
    ExprPtr e = parse_primary();
    while (check(TokenKind::Operator, "*") || check(TokenKind::Operator, "/")) {
        std::string op = advance().text;
        auto b = std::make_shared<BinaryExpr>();
        b->op = op;
        b->left = e;
        b->right = parse_primary();
        e = b;
    }
    return e;
}

ExprPtr Parser::parse_primary() {
    if (check(TokenKind::Numeric)) {
        auto lit = std::make_shared<Literal>();
        lit->number = std::stod(advance().text);
        return lit;
    }
    if (check(TokenKind::String)) {
        auto lit = std::make_shared<Literal>();
        lit->is_string = true;
        lit->str = advance().text;
        return lit;
    }
    if (check(TokenKind::Keyword, "true")) {
        advance();
        auto lit = std::make_shared<Literal>();
        lit->number = 1;
        return lit;
    }
    if (check(TokenKind::Keyword, "false") || check(TokenKind::Keyword, "null")) {
        advance();
        return std::make_shared<Literal>(); // 0
    }
    if (check(TokenKind::Identifier)) {
        std::string name = advance().text;
        if (match(TokenKind::Punct, "(")) {
            auto call = std::make_shared<CallExpr>();
            call->callee = name;
            if (!check(TokenKind::Punct, ")")) {
                do {
                    call->args.push_back(parse_expr());
                } while (match(TokenKind::Punct, ","));
            }
            expect(TokenKind::Punct, ")");
            return call;
        }
        return std::make_shared<Identifier>(name);
    }
    if (match(TokenKind::Punct, "(")) {
        ExprPtr e = parse_expr();
        expect(TokenKind::Punct, ")");
        return e;
    }
    throw ParseError("unexpected token '" + peek().text + "'");
}

} // namespace charcoal1
