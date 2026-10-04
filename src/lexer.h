#pragma once
// Charcoal1 spaceless lexer: maximal-munch / keyword-first greedy matching.
// Handles sources with ZERO whitespace (e.g. "letx=1;functionfoo(){return1;}")
// via a keyword trie, symbol boundary triggers, and grammar context tracking.

#include <string>
#include <vector>

namespace charcoal1 {

enum class TokenKind {
    Keyword,     // let, const, var, function, return, if, else, while, ...
    Identifier,  // variable / function names
    Numeric,     // 123, 4.5
    String,      // "text"
    Operator,    // = + - * / ! < > == != <= >= && ||
    Punct,       // , ; . ( ) { } [ ]
    Eof,
};

struct Token {
    TokenKind kind = TokenKind::Eof;
    std::string text;
};

std::string token_kind_name(TokenKind k);

class Lexer {
public:
    explicit Lexer(const std::string& source);
    Token next_token();
    std::vector<Token> tokenize_all();

private:
    // Longest keyword-trie match at pos_; 0 if none.
    size_t keyword_match(size_t pos) const;
    // Operator match length at pos_ (0 if none); out receives the text.
    size_t operator_match(size_t pos, std::string& out) const;
    static bool is_op_char(char c);

    struct TrieNode {
        int child[128];
        bool terminal = false;
        TrieNode() { for (int i = 0; i < 128; ++i) child[i] = -1; }
    };
    static const std::vector<TrieNode>& keyword_trie();

    std::string src_;
    size_t pos_ = 0;
    // Grammar context: after function/let/const/var, the next token is forced
    // to be an identifier (so "functionreturn" lexes as one name, not keyword).
    bool context_forces_identifier_ = false;
};

} // namespace charcoal1
