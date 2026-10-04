// Charcoal1 lexer tests (assert-based).
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "lexer.h"

using namespace charcoal1;

static int failures = 0;
#define CHECK(cond) \
    do { if (!(cond)) { std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; failures++; } } while (0)

static std::vector<Token> toks(const std::string& s) {
    Lexer lx(s);
    return lx.tokenize_all();
}

static std::string kinds(const std::vector<Token>& ts) {
    std::string out;
    for (auto& t : ts) {
        if (!out.empty()) out += " ";
        out += token_kind_name(t.kind) + ":" + t.text;
    }
    return out;
}

void test_spaceless_let() {
    auto ts = toks("letx=1;");
    // Keyword(let) Identifier(x) Operator(=) Numeric(1) Punct(;) Eof
    CHECK(ts.size() == 6);
    CHECK(ts[0].kind == TokenKind::Keyword && ts[0].text == "let");
    CHECK(ts[1].kind == TokenKind::Identifier && ts[1].text == "x");
    CHECK(ts[2].kind == TokenKind::Operator && ts[2].text == "=");
    CHECK(ts[3].kind == TokenKind::Numeric && ts[3].text == "1");
}

void test_spaceless_function() {
    auto ts = toks("functionfoo(){return1;}");
    CHECK(ts[0].kind == TokenKind::Keyword && ts[0].text == "function");
    CHECK(ts[1].kind == TokenKind::Identifier && ts[1].text == "foo");
    // find return keyword and numeric 1
    bool saw_return = false, saw_one = false;
    for (auto& t : ts) {
        if (t.kind == TokenKind::Keyword && t.text == "return") saw_return = true;
        if (t.kind == TokenKind::Numeric && t.text == "1") saw_one = true;
    }
    CHECK(saw_return);
    CHECK(saw_one);
}

void test_function_named_like_keyword() {
    // "function" followed by "returnvalue": context forces one identifier
    auto ts = toks("functionreturnvalue(){}");
    CHECK(ts[0].kind == TokenKind::Keyword && ts[0].text == "function");
    CHECK(ts[1].kind == TokenKind::Identifier && ts[1].text == "returnvalue");
}

void test_operators_and_strings() {
    auto ts = toks("if(a<=b){print(\"x\");}");
    bool saw_le = false, saw_str = false;
    for (auto& t : ts) {
        if (t.kind == TokenKind::Operator && t.text == "<=") saw_le = true;
        if (t.kind == TokenKind::String && t.text == "x") saw_str = true;
    }
    CHECK(saw_le);
    CHECK(saw_str);
}

void test_spaced_source_still_works() {
    auto ts = toks("let count = 10;");
    CHECK(ts[0].kind == TokenKind::Keyword && ts[0].text == "let");
    CHECK(ts[1].kind == TokenKind::Identifier && ts[1].text == "count");
    CHECK(ts[3].kind == TokenKind::Numeric && ts[3].text == "10");
}

int main() {
    test_spaceless_let();
    test_spaceless_function();
    test_function_named_like_keyword();
    test_operators_and_strings();
    test_spaced_source_still_works();
    if (failures == 0) std::cout << "all lexer tests passed\n";
    else std::cout << kinds(toks("letx=1;")) << "\n";
    return failures == 0 ? 0 : 1;
}
