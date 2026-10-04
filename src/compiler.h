#pragma once
// Charcoal1 compiler: AST -> bytecode Program.

#include <map>
#include <string>
#include <vector>

#include "ast.h"
#include "bytecode.h"

namespace charcoal1 {

class Compiler {
public:
    // Compiles top-level statements. Function declarations are hoisted into
    // the function table; remaining statements form "main".
    Program compile(const std::vector<StmtPtr>& program);

private:
    struct Ctx {
        Function* func;
        std::map<std::string, size_t> locals; // name -> slot
    };

    size_t add_constant(Ctx& ctx, const Value& v);
    size_t local_slot(Ctx& ctx, const std::string& name);

    void emit(Ctx& ctx, Op op);
    void emit_u16(Ctx& ctx, Op op, uint16_t operand);
    // Emits a jump with a placeholder target; returns patch position.
    size_t emit_jump(Ctx& ctx, Op op);
    void patch_jump(Ctx& ctx, size_t pos);

    void compile_stmt(Ctx& ctx, const StmtPtr& s);
    void compile_expr(Ctx& ctx, const ExprPtr& e);
    void compile_function(const std::shared_ptr<FuncDecl>& fd);

    Program prog_;
    std::map<std::string, size_t> func_index_; // name -> functions[] index
};

} // namespace charcoal1
