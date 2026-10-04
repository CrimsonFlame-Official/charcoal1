// Charcoal1 compiler implementation.
#include "compiler.h"

#include <stdexcept>

namespace charcoal1 {

void Compiler::register_native(const std::string& name) {
    if (native_index_.count(name)) return;
    size_t idx = prog_.native_names.size();
    prog_.native_names.push_back(name);
    native_index_[name] = idx;
}

Program Compiler::compile(const std::vector<StmtPtr>& program) {
    prog_.functions.clear();
    func_index_.clear();

    // main() is always functions[0]
    prog_.functions.push_back(Function{"main", {}, Code{}});
    func_index_["main"] = 0;

    // hoist function declarations first (so calls can precede definitions)
    for (auto& s : program) {
        if (auto* fd = dynamic_cast<FuncDecl*>(s.get())) {
            size_t idx = prog_.functions.size();
            func_index_[fd->name] = idx;
            prog_.functions.push_back(Function{fd->name, fd->params, Code{}});
        }
    }
    for (auto& s : program) {
        if (auto* fd = dynamic_cast<FuncDecl*>(s.get())) compile_function(
            std::static_pointer_cast<FuncDecl>(s));
    }

    // remaining top-level statements -> main
    Ctx ctx{&prog_.functions[0], {}};
    for (auto& s : program) {
        if (dynamic_cast<FuncDecl*>(s.get())) continue;
        compile_stmt(ctx, s);
    }
    // main returns the top of stack (or null if empty -- VM handles it)
    emit(ctx, Op::OP_HALT);
    prog_.functions[0].code.local_count = ctx.locals.size();
    return std::move(prog_);
}

void Compiler::compile_function(const std::shared_ptr<FuncDecl>& fd) {
    size_t idx = func_index_.at(fd->name);
    Function& fn = prog_.functions[idx];
    Ctx ctx{&fn, {}};
    for (auto& p : fd->params) local_slot(ctx, p); // params occupy slots 0..n
    for (auto& s : fd->body) compile_stmt(ctx, s);
    // implicit return null if no explicit return
    emit_u16(ctx, Op::OP_LOAD_CONST,
             static_cast<uint16_t>(add_constant(ctx, Value::null())));
    emit(ctx, Op::OP_RET);
    fn.code.local_count = ctx.locals.size();
}

size_t Compiler::add_constant(Ctx& ctx, const Value& v) {
    ctx.func->code.constants.push_back(v);
    return ctx.func->code.constants.size() - 1;
}

size_t Compiler::local_slot(Ctx& ctx, const std::string& name) {
    auto it = ctx.locals.find(name);
    if (it != ctx.locals.end()) return it->second;
    size_t slot = ctx.locals.size();
    ctx.locals[name] = slot;
    return slot;
}

void Compiler::emit(Ctx& ctx, Op op) {
    ctx.func->code.bytes.push_back(static_cast<uint8_t>(op));
}

void Compiler::emit_u16(Ctx& ctx, Op op, uint16_t operand) {
    emit(ctx, op);
    ctx.func->code.bytes.push_back(static_cast<uint8_t>(operand >> 8));
    ctx.func->code.bytes.push_back(static_cast<uint8_t>(operand & 0xff));
}

size_t Compiler::emit_jump(Ctx& ctx, Op op) {
    emit(ctx, op);
    size_t pos = ctx.func->code.bytes.size();
    ctx.func->code.bytes.push_back(0);
    ctx.func->code.bytes.push_back(0);
    return pos;
}

void Compiler::patch_jump(Ctx& ctx, size_t pos) {
    uint16_t target = static_cast<uint16_t>(ctx.func->code.bytes.size());
    ctx.func->code.bytes[pos] = static_cast<uint8_t>(target >> 8);
    ctx.func->code.bytes[pos + 1] = static_cast<uint8_t>(target & 0xff);
}

void Compiler::compile_stmt(Ctx& ctx, const StmtPtr& s) {
    if (auto* vd = dynamic_cast<VarDecl*>(s.get())) {
        size_t slot = local_slot(ctx, vd->name);
        if (vd->init) compile_expr(ctx, vd->init);
        else
            emit_u16(ctx, Op::OP_LOAD_CONST,
                     static_cast<uint16_t>(add_constant(ctx, Value::null())));
        emit_u16(ctx, Op::OP_STORE_LOCAL, static_cast<uint16_t>(slot));
    } else if (auto* es = dynamic_cast<ExprStmt*>(s.get())) {
        compile_expr(ctx, es->expr);
        emit(ctx, Op::OP_POP);
    } else if (auto* rs = dynamic_cast<ReturnStmt*>(s.get())) {
        if (rs->value) compile_expr(ctx, rs->value);
        else
            emit_u16(ctx, Op::OP_LOAD_CONST,
                     static_cast<uint16_t>(add_constant(ctx, Value::null())));
        emit(ctx, Op::OP_RET);
    } else if (auto* is = dynamic_cast<IfStmt*>(s.get())) {
        compile_expr(ctx, is->cond);
        size_t else_jump = emit_jump(ctx, Op::OP_JUMP_IF_FALSE);
        for (auto& st : is->then_body) compile_stmt(ctx, st);
        if (is->else_body.empty()) {
            patch_jump(ctx, else_jump);
        } else {
            size_t end_jump = emit_jump(ctx, Op::OP_JUMP);
            patch_jump(ctx, else_jump);
            for (auto& st : is->else_body) compile_stmt(ctx, st);
            patch_jump(ctx, end_jump);
        }
    } else if (auto* ws = dynamic_cast<WhileStmt*>(s.get())) {
        size_t loop_start = ctx.func->code.bytes.size();
        compile_expr(ctx, ws->cond);
        size_t end_jump = emit_jump(ctx, Op::OP_JUMP_IF_FALSE);
        for (auto& st : ws->body) compile_stmt(ctx, st);
        emit_u16(ctx, Op::OP_JUMP, static_cast<uint16_t>(loop_start));
        patch_jump(ctx, end_jump);
    } else {
        throw std::runtime_error("compiler: unhandled statement");
    }
}

void Compiler::compile_expr(Ctx& ctx, const ExprPtr& e) {
    if (auto* lit = dynamic_cast<Literal*>(e.get())) {
        if (lit->is_string) {
            size_t idx = prog_.strings.size();
            prog_.strings.push_back(lit->str);
            emit_u16(ctx, Op::OP_LOAD_STRING, static_cast<uint16_t>(idx));
        } else {
            size_t idx = add_constant(ctx, Value::num(lit->number));
            emit_u16(ctx, Op::OP_LOAD_CONST, static_cast<uint16_t>(idx));
        }
    } else if (auto* id = dynamic_cast<Identifier*>(e.get())) {
        auto it = ctx.locals.find(id->name);
        if (it == ctx.locals.end())
            throw std::runtime_error("compiler: undefined variable '" + id->name + "'");
        emit_u16(ctx, Op::OP_LOAD_LOCAL, static_cast<uint16_t>(it->second));
    } else if (auto* b = dynamic_cast<BinaryExpr*>(e.get())) {
        if (b->op == "=") {
            auto* id = dynamic_cast<Identifier*>(b->left.get());
            if (!id) throw std::runtime_error("compiler: bad assignment target");
            compile_expr(ctx, b->right);
            size_t slot = local_slot(ctx, id->name);
            emit_u16(ctx, Op::OP_STORE_LOCAL, static_cast<uint16_t>(slot));
            // an assignment expression yields the assigned value
            emit_u16(ctx, Op::OP_LOAD_LOCAL, static_cast<uint16_t>(slot));
        } else {
            compile_expr(ctx, b->left);
            compile_expr(ctx, b->right);
            if (b->op == "+") emit(ctx, Op::OP_ADD);
            else if (b->op == "-") emit(ctx, Op::OP_SUB);
            else if (b->op == "*") emit(ctx, Op::OP_MUL);
            else if (b->op == "/") emit(ctx, Op::OP_DIV);
            else if (b->op == "==") emit(ctx, Op::OP_EQ);
            else if (b->op == "!=") emit(ctx, Op::OP_NE);
            else if (b->op == "<") emit(ctx, Op::OP_LT);
            else if (b->op == ">") emit(ctx, Op::OP_GT);
            else if (b->op == "<=") emit(ctx, Op::OP_LE);
            else if (b->op == ">=") emit(ctx, Op::OP_GE);
            else throw std::runtime_error("compiler: unknown operator '" + b->op + "'");
        }
    } else if (auto* c = dynamic_cast<CallExpr*>(e.get())) {
        if (c->callee == "print") {
            if (c->args.size() != 1)
                throw std::runtime_error("compiler: print() takes 1 argument");
            compile_expr(ctx, c->args[0]);
            emit(ctx, Op::OP_PRINT);
            emit_u16(ctx, Op::OP_LOAD_CONST,
                     static_cast<uint16_t>(add_constant(ctx, Value::null())));
        } else {
            auto it = func_index_.find(c->callee);
            if (it != func_index_.end()) {
                for (auto& a : c->args) compile_expr(ctx, a);
                emit_u16(ctx, Op::OP_CALL, static_cast<uint16_t>(it->second));
                ctx.func->code.bytes.push_back(static_cast<uint8_t>(c->args.size()));
            } else {
                auto nit = native_index_.find(c->callee);
                if (nit == native_index_.end())
                    throw std::runtime_error("compiler: undefined function '" +
                                             c->callee + "'");
                for (auto& a : c->args) compile_expr(ctx, a);
                emit_u16(ctx, Op::OP_CALL_NATIVE, static_cast<uint16_t>(nit->second));
                ctx.func->code.bytes.push_back(static_cast<uint8_t>(c->args.size()));
            }
        }
    } else {
        throw std::runtime_error("compiler: unhandled expression");
    }
}

} // namespace charcoal1
