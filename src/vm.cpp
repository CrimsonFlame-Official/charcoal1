// Charcoal1 VM implementation.
#include "vm.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace charcoal1 {

std::string Value::to_string() const {
    switch (type) {
        case Type::Null:   return "null";
        case Type::Number: {
            double i;
            if (std::modf(number, &i) == 0) return std::to_string(static_cast<long long>(i));
            return std::to_string(number);
        }
        case Type::Bool:   return number != 0 ? "true" : "false";
        case Type::Object:
            if (auto* s = dynamic_cast<GcString*>(object)) return s->value;
            return "[object]";
        case Type::HostObject: return "[host object]";
    }
    return "null";
}

VM::VM(Program& prog) : prog_(prog) {}

uint16_t VM::read_u16(Frame& f) {
    uint16_t v = static_cast<uint16_t>(f.func->code.bytes[f.ip] << 8) |
                 static_cast<uint16_t>(f.func->code.bytes[f.ip + 1]);
    f.ip += 2;
    return v;
}

std::vector<GcObject*> VM::gc_roots() {
    std::vector<GcObject*> roots;
    for (auto& v : stack_)
        if (v.type == Value::Type::Object && v.object) roots.push_back(v.object);
    for (auto& fr : frames_)
        for (auto& v : fr.locals)
            if (v.type == Value::Type::Object && v.object) roots.push_back(v.object);
    for (auto& fn : prog_.functions)
        for (auto& v : fn.code.constants)
            if (v.type == Value::Type::Object && v.object) roots.push_back(v.object);
    return roots;
}

void VM::maybe_collect() {
    // Collect every 512 instructions at a safe point (between opcodes).
    if (++instr_count_ % 512 == 0) heap_.collect(gc_roots());
}

static double num_of(const Value& v) {
    if (v.type == Value::Type::Number || v.type == Value::Type::Bool) return v.number;
    return 0;
}

Value VM::run() {
    Function& main = prog_.functions[0];
    frames_.push_back(Frame{&main, 0, std::vector<Value>(main.code.local_count)});
    run_until(0);
    return stack_.empty() ? Value::null() : stack_.back();
}

Value VM::call_function(const std::string& name, const std::vector<Value>& args) {
    size_t idx = prog_.functions.size();
    for (size_t i = 0; i < prog_.functions.size(); ++i) {
        if (prog_.functions[i].name == name) { idx = i; break; }
    }
    if (idx >= prog_.functions.size())
        throw std::runtime_error("vm: no such function '" + name + "'");
    Function& callee = prog_.functions[idx];
    Frame fr{&callee, 0, std::vector<Value>(callee.code.local_count)};
    for (size_t i = 0; i < args.size() && i < fr.locals.size(); ++i)
        fr.locals[i] = args[i];
    size_t depth = frames_.size();
    frames_.push_back(std::move(fr));
    run_until(depth);
    return stack_.empty() ? Value::null() : pop();
}

void VM::register_native(const std::string& name, NativeFn fn) {
    natives_[name] = std::move(fn);
}

void VM::run_until(size_t depth) {
    while (frames_.size() > depth) {
        Frame& f = frames_.back();
        if (f.ip >= f.func->code.bytes.size())
            throw std::runtime_error("vm: ip out of bounds");
        Op op = static_cast<Op>(f.func->code.bytes[f.ip++]);
        maybe_collect();

        switch (op) {
            case Op::OP_LOAD_CONST: {
                uint16_t i = read_u16(f);
                push(f.func->code.constants[i]);
                break;
            }
            case Op::OP_LOAD_STRING: {
                uint16_t i = read_u16(f);
                push(Value::str(heap_.allocate<GcString>(prog_.strings[i])));
                break;
            }
            case Op::OP_LOAD_LOCAL: {
                uint16_t s = read_u16(f);
                push(f.locals[s]);
                break;
            }
            case Op::OP_STORE_LOCAL: {
                uint16_t s = read_u16(f);
                f.locals[s] = pop();
                break;
            }
            case Op::OP_ADD:
            case Op::OP_SUB:
            case Op::OP_MUL:
            case Op::OP_DIV: {
                Value b = pop(), a = pop();
                double r = 0;
                if (op == Op::OP_ADD) r = num_of(a) + num_of(b);
                else if (op == Op::OP_SUB) r = num_of(a) - num_of(b);
                else if (op == Op::OP_MUL) r = num_of(a) * num_of(b);
                else r = num_of(a) / num_of(b);
                push(Value::num(r));
                break;
            }
            case Op::OP_EQ:
            case Op::OP_NE:
            case Op::OP_LT:
            case Op::OP_GT:
            case Op::OP_LE:
            case Op::OP_GE: {
                Value b = pop(), a = pop();
                bool r = false;
                double x = num_of(a), y = num_of(b);
                if (op == Op::OP_EQ) r = (x == y);
                else if (op == Op::OP_NE) r = (x != y);
                else if (op == Op::OP_LT) r = (x < y);
                else if (op == Op::OP_GT) r = (x > y);
                else if (op == Op::OP_LE) r = (x <= y);
                else r = (x >= y);
                push(Value::boolean(r));
                break;
            }
            case Op::OP_JUMP: {
                f.ip = read_u16(f);
                break;
            }
            case Op::OP_JUMP_IF_FALSE: {
                uint16_t t = read_u16(f);
                if (!pop().truthy()) f.ip = t;
                break;
            }
            case Op::OP_CALL: {
                uint16_t fi = read_u16(f);
                uint8_t argc = f.func->code.bytes[f.ip++];
                if (fi >= prog_.functions.size())
                    throw std::runtime_error("vm: bad function index");
                Function& callee = prog_.functions[fi];
                Frame nf{&callee, 0, std::vector<Value>(callee.code.local_count)};
                for (int i = argc - 1; i >= 0; --i) nf.locals[i] = pop();
                frames_.push_back(std::move(nf));
                break;
            }
            case Op::OP_CALL_NATIVE: {
                uint16_t ni = read_u16(f);
                uint8_t argc = f.func->code.bytes[f.ip++];
                if (ni >= prog_.native_names.size())
                    throw std::runtime_error("vm: bad native index");
                const std::string& name = prog_.native_names[ni];
                auto it = natives_.find(name);
                if (it == natives_.end())
                    throw std::runtime_error("vm: native '" + name + "' not registered");
                std::vector<Value> args(argc);
                for (int i = argc - 1; i >= 0; --i) args[i] = pop();
                push(it->second(*this, args));
                break;
            }
            case Op::OP_PRINT: {
                std::cout << pop().to_string() << "\n";
                break;
            }
            case Op::OP_POP: {
                pop();
                break;
            }
            case Op::OP_RET: {
                Value ret = pop();
                frames_.pop_back();
                if (!frames_.empty()) push(ret);
                else if (!stack_.empty()) { /* main returned: keep value */ push(ret); }
                break;
            }
            case Op::OP_HALT: {
                // Unwinds to the base depth; run() reads the stack top.
                frames_.clear();
                break;
            }
        }
    }
}

std::string disassemble(const Program& prog) {
    std::string out;
    for (size_t fi = 0; fi < prog.functions.size(); ++fi) {
        auto& fn = prog.functions[fi];
        out += "== function " + fn.name + " (" +
               std::to_string(fn.code.bytes.size()) + " bytes, " +
               std::to_string(fn.code.local_count) + " locals) ==\n";
        size_t ip = 0;
        auto& b = fn.code.bytes;
        static const char* names[] = {
            "LOAD_CONST", "LOAD_LOCAL", "STORE_LOCAL", "ADD", "SUB", "MUL",
            "DIV", "EQ", "NE", "LT", "GT", "LE", "GE", "JUMP",
            "JUMP_IF_FALSE", "CALL", "CALL_NATIVE", "LOAD_STRING", "PRINT",
            "POP", "RET", "HALT",
        };
        while (ip < b.size()) {
            char line[64];
            uint8_t op = b[ip];
            const char* nm = op < 22 ? names[op] : "?";
            std::snprintf(line, sizeof(line), "  %4zu  %-14s", ip, nm);
            out += line;
            ++ip;
            // operands
            if (op == Op::OP_LOAD_CONST || op == Op::OP_LOAD_LOCAL ||
                op == Op::OP_STORE_LOCAL || op == Op::OP_JUMP ||
                op == Op::OP_JUMP_IF_FALSE || op == Op::OP_LOAD_STRING) {
                uint16_t v = static_cast<uint16_t>(b[ip] << 8) | b[ip + 1];
                out += " " + std::to_string(v);
                ip += 2;
            } else if (op == Op::OP_CALL || op == Op::OP_CALL_NATIVE) {
                uint16_t fi2 = static_cast<uint16_t>(b[ip] << 8) | b[ip + 1];
                uint8_t argc = b[ip + 2];
                out += " f" + std::to_string(fi2) + " argc=" + std::to_string(argc);
                ip += 3;
            }
            out += "\n";
        }
    }
    return out;
}

} // namespace charcoal1
