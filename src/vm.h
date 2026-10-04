#pragma once
// Charcoal1 VM: stack-based interpreter over Charcoal1 bytecode.
// Embedders register native functions (host calls) by name; scripts invoke
// them via OP_CALL_NATIVE. call_function() lets the host invoke script
// functions after run() (e.g. timer callbacks from the event loop).

#include <functional>
#include <map>
#include <string>
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
    using NativeFn = std::function<Value(VM&, const std::vector<Value>&)>;

    explicit VM(Program& prog);

    // Embedder API: expose a host function to scripts under `name`.
    // The compiler must have registered the same name via
    // Compiler::register_native().
    void register_native(const std::string& name, NativeFn fn);

    // Runs functions[0] ("main"). Returns the stack top at HALT
    // (null when the stack is empty).
    Value run();

    // Invokes a script function by name (must exist in the program).
    // Used by the embedder event loop for deferred callbacks.
    Value call_function(const std::string& name, const std::vector<Value>& args);

    GcHeap& heap() { return heap_; }

private:
    uint16_t read_u16(Frame& f);
    void push(const Value& v) { stack_.push_back(v); }
    Value pop() {
        Value v = stack_.back();
        stack_.pop_back();
        return v;
    }
    // Runs until the frame stack shrinks back to `depth`.
    void run_until(size_t depth);
    // Snapshot every heap object reachable from the stack and frames.
    std::vector<GcObject*> gc_roots();
    void maybe_collect();

    Program& prog_;
    GcHeap heap_;
    std::vector<Value> stack_;
    std::vector<Frame> frames_;
    std::map<std::string, NativeFn> natives_;
    size_t instr_count_ = 0;
};

} // namespace charcoal1
