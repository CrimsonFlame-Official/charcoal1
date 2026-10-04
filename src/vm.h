#pragma once
// Charcoal1 VM: stack-based interpreter over Charcoal1 bytecode.

#include <vector>

#include "bytecode.h"

namespace charcoal1 {

struct Frame {
    Function* func = nullptr;
    size_t ip = 0;
    std::vector<Value> locals;
};

class VM {
public:
    explicit VM(Program& prog);

    // Runs functions[0] ("main"). Returns the stack top at HALT
    // (null when the stack is empty).
    Value run();

    GcHeap& heap() { return heap_; }

private:
    uint16_t read_u16(Frame& f);
    void push(const Value& v) { stack_.push_back(v); }
    Value pop() {
        Value v = stack_.back();
        stack_.pop_back();
        return v;
    }
    // Snapshot every heap object reachable from the stack and frames.
    std::vector<GcObject*> gc_roots();
    void maybe_collect();

    Program& prog_;
    GcHeap heap_;
    std::vector<Value> stack_;
    std::vector<Frame> frames_;
    size_t instr_count_ = 0;
};

} // namespace charcoal1
