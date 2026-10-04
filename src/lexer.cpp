// Charcoal1 spaceless lexer implementation.
#include "lexer.h"

namespace charcoal1 {

std::string token_kind_name(TokenKind k) {
    switch (k) {
        case TokenKind::Keyword:    return "Keyword";
        case TokenKind::Identifier: return "Identifier";
        case TokenKind::Numeric:    return "Numeric";
        case TokenKind::String:     return "String";
        case TokenKind::Operator:   return "Operator";
        case TokenKind::Punct:      return "Punct";
        case TokenKind::Eof:        return "Eof";
    }
    return "?";
}

const std::vector<Lexer::TrieNode>& Lexer::keyword_trie() {
    static std::vector<TrieNode> trie = [] {
        std::vector<TrieNode> t(1); // root at 0
        const char* kws[] = {
            "let", "const", "var", "function", "return", "if", "else",
            "while", "for", "true", "false", "null", "undefined", "new",
            "this", "typeof", "do", nullptr
        };
        for (int k = 0; kws[k]; ++k) {
            int node = 0;
            for (const char* p = kws[k]; *p; ++p) {
                unsigned char c = static_cast<unsigned char>(*p);
                if (t[node].child[c] == -1) {
                    t[node].child[c] = static_cast<int>(t.size());
                    t.emplace_back();
                }
                node = t[node].child[c];
            }
            t[node].terminal = true;
        }
        return t;
    }();
    return trie;
}

Lexer::Lexer(const std::string& source) : src_(source) {}

size_t Lexer::keyword_match(size_t pos) const {
    const auto& trie = keyword_trie();
    int node = 0;
    size_t best = 0;
    for (size_t i = pos; i < src_.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(src_[i]);
        if (c >= 128 || trie[node].child[c] == -1) break;
        node = trie[node].child[c];
        if (trie[node].terminal) best = i - pos + 1;
    }
    return best;
}

bool Lexer::is_op_char(char c) {
    return c == '=' || c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '!' || c == '<' || c == '>' || c == '&' || c == '|';
}

size_t Lexer::operator_match(size_t pos, std::string& out) const {
    if (pos >= src_.size() || !is_op_char(src_[pos])) return 0;
    // try two-char operators first
    static const char* two[] = {"==", "!=", "<=", ">=", "&&", "||", nullptr};
    if (pos + 1 < src_.size()) {
        std::string cand = src_.substr(pos, 2);
        for (int i = 0; two[i]; ++i)
            if (cand == two[i]) { out = cand; return 2; }
    }
    out = src_.substr(pos, 1);
    return 1;
}

static bool is_punct(char c) {
    return c == ',' || c == ';' || c == '.' || c == '(' || c == ')' ||
           c == '{' || c == '}' || c == '[' || c == ']';
}

static bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$';
}

Token Lexer::next_token() {
    // whitespace is insignificant but still skipped when present
    while (pos_ < src_.size() &&
           std::isspace(static_cast<unsigned char>(src_[pos_])))
        ++pos_;
    if (pos_ >= src_.size()) return Token{TokenKind::Eof, ""};

    // 1. Keyword trie match (unless grammar context forces an identifier)
    if (!context_forces_identifier_) {
        size_t kw_len = keyword_match(pos_);
        if (kw_len > 0) {
            std::string kw = src_.substr(pos_, kw_len);
            pos_ += kw_len;
            if (kw == "function" || kw == "let" || kw == "const" || kw == "var")
                context_forces_identifier_ = true;
            return Token{TokenKind::Keyword, kw};
        }
    }

    char c = src_[pos_];

    // 2. Operators (strict boundary triggers)
    std::string op;
    size_t op_len = operator_match(pos_, op);
    if (op_len > 0) {
        pos_ += op_len;
        context_forces_identifier_ = false;
        return Token{TokenKind::Operator, op};
    }

    // 3. Punctuation
    if (is_punct(c)) {
        pos_++;
        context_forces_identifier_ = false;
        return Token{TokenKind::Punct, std::string(1, c)};
    }

    // 4. String literals
    if (c == '"' || c == '\'') {
        char q = c;
        std::string val;
        pos_++; // open quote
        while (pos_ < src_.size() && src_[pos_] != q) {
            if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
                val += src_[pos_ + 1];
                pos_ += 2;
            } else {
                val += src_[pos_++];
            }
        }
        if (pos_ < src_.size()) pos_++; // close quote
        context_forces_identifier_ = false;
        return Token{TokenKind::String, val};
    }

    // 5. Numeric literals
    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string num;
        while (pos_ < src_.size() &&
               (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.'))
            num += src_[pos_++];
        context_forces_identifier_ = false;
        return Token{TokenKind::Numeric, num};
    }

    // 6. Identifiers: maximal munch until a structural boundary.
    // (Keyword matching already ran at token start in step 1, so a keyword
    // embedded mid-identifier like "xletter" stays one identifier.)
    std::string id;
    while (pos_ < src_.size() && is_ident_char(src_[pos_]))
        id += src_[pos_++];
    context_forces_identifier_ = false;
    if (!id.empty()) return Token{TokenKind::Identifier, id};

    // unknown char: skip it
    pos_++;
    return next_token();
}

std::vector<Token> Lexer::tokenize_all() {
    std::vector<Token> out;
    for (Token t = next_token(); t.kind != TokenKind::Eof; t = next_token())
        out.push_back(t);
    out.push_back(Token{TokenKind::Eof, ""});
    return out;
}

} // namespace charcoal1
