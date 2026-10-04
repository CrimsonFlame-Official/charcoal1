#pragma once
// Charcoal1 bytecode ISA + value model.
// Stack-based: operations pop operands and push results.

#include <cstdint>
#include <string>
#include <vector>

#include "gc.h"

namespace charcoal1 {

// --- values ----------------------------------------------------------------
struct Value {
    enum class Type { Null, Number, Bool, Object, HostObject };
    Type type = Type::Null;
    double number = 0;
    GcObject* object = nullptr; // GcString for strings
    void* host = nullptr;       // HostObject: native pointer owned by the embedder
                                // (e.g. an Ignite DOM node). Not GC-managed.

    static Value null() { return Value{}; }
    static Value num(double d) { Value v; v.type = Type::Number; v.number = d; return v; }
    static Value boolean(bool b) {
        Value v; v.type = Type::Bool; v.number = b ? 1 : 0; return v;
    }
    static Value str(GcString* s) { Value v; v.type = Type::Object; v.object = s; return v; }
    static Value host_object(void* p) { Value v; v.type = Type::HostObject; v.host = p; return v; }

    bool truthy() const {
        switch (type) {
            case Type::Null:   return false;
            case Type::Bool:
            case Type::Number: return number != 0;
            case Type::Object:
            case Type::HostObject: return true;
        }
        return false;
    }
    std::string to_string() const;
};

// --- instruction set --------------------------------------------------------
enum Op : uint8_t {
    OP_LOAD_CONST,   // u16 const_idx        -> push constants[i]
    OP_LOAD_LOCAL,   // u16 slot             -> push frame.locals[i]
    OP_STORE_LOCAL,  // u16 slot             -> frame.locals[i] = pop()
    OP_ADD,          // pop b,a; push a+b
    OP_SUB,          // pop b,a; push a-b
    OP_MUL,          // pop b,a; push a*b
    OP_DIV,          // pop b,a; push a/b
    OP_EQ,           // pop b,a; push (a==b)
    OP_NE,           // pop b,a; push (a!=b)
    OP_LT,           // pop b,a; push (a<b)
    OP_GT,           // pop b,a; push (a>b)
    OP_LE,           // pop b,a; push (a<=b)
    OP_GE,           // pop b,a; push (a>=b)
    OP_JUMP,         // u16 target           -> ip = target
    OP_JUMP_IF_FALSE,// u16 target           -> pop(); if falsy ip = target
    OP_CALL,         // u16 func_idx, u8 argc -> call function
    OP_CALL_NATIVE,  // u16 native_idx, u8 argc -> call embedder native
    OP_LOAD_STRING,  // u16 string_idx       -> push program.strings[i]
    OP_PRINT,        // pop(); print to stdout
    OP_POP,          // discard top of stack
    OP_RET,          // return from frame (return value on stack top)
    OP_HALT,
};

struct Code {
    std::vector<uint8_t> bytes;
    std::vector<Value> constants;
    size_t local_count = 0;
};

struct Function {
    std::string name;
    std::vector<std::string> params;
    Code code;
};

struct Program {
    std::vector<Function> functions; // [0] is always "main"
    std::vector<std::string> strings; // string constant pool
    std::vector<std::string> native_names; // embedder natives, by OP_CALL_NATIVE index
};

std::string disassemble(const Program& prog);

} // namespace charcoal1
