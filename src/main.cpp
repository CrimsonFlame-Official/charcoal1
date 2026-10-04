// Charcoal1 demo: lex -> parse -> compile -> run, all with zero spaces.
#include <iostream>
#include <string>

#include "bytecode.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

static void run_source(const std::string& src, bool dump) {
    charcoal1::Lexer lx(src);
    auto toks = lx.tokenize_all();
    if (dump) {
        std::cout << "--- tokens ---\n";
        for (auto& t : toks)
            std::cout << charcoal1::token_kind_name(t.kind) << "('" << t.text << "')\n";
    }
    charcoal1::Parser p(std::move(toks));
    auto prog = p.parse_program();
    if (dump) {
        std::cout << "--- AST ---\n";
        for (auto& s : prog) std::cout << s->debug() << "\n";
    }
    charcoal1::Compiler c;
    charcoal1::Program bc = c.compile(prog);
    if (dump) std::cout << "--- bytecode ---\n" << charcoal1::disassemble(bc);
    charcoal1::VM vm(bc);
    std::cout << "--- run ---\n";
    charcoal1::Value r = vm.run();
    std::cout << "=> " << r.to_string() << "\n";
    std::cout << "(gc heap live objects: " << vm.heap().live_count() << ")\n\n";
}

int main() {
    // No spaces anywhere in these sources.
    run_source("letx=1;lety=2;print(x+y*3);", true);
    run_source("functionadd(a,b){returna+b;}print(add(40,2));", false);
    run_source("leti=0;while(i<5){print(i);i=i+1;}", false);
    run_source("letn=7;if(n>10){print(\"big\");}else{print(\"small\");}", false);
    // And the same language with normal spacing still works.
    run_source("let s = \"hello\"; print(s);", false);
    return 0;
}
