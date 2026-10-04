#pragma once
// Charcoal1 mark-and-sweep GC.
// Roots: VM evaluation stack + active call frames + global scope.
// Lifecycle: mark reachable from roots, sweep the rest back to the free list.

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace charcoal1 {

class GcHeap;

class GcObject {
public:
    bool marked = false;
    virtual ~GcObject() = default;
    virtual void trace(GcHeap& heap) = 0; // mark outgoing references
};

class GcString : public GcObject {
public:
    explicit GcString(std::string v) : value(std::move(v)) {}
    void trace(GcHeap&) override {} // strings hold no references
    std::string value;
};

class GcHeap {
public:
    ~GcHeap();

    template <typename T, typename... Args>
    T* allocate(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        objects_.push_back(obj);
        return obj;
    }

    // Roots are passed explicitly: the VM snapshots every heap object
    // reachable from its stack, frames, and globals, then collects.
    void collect(const std::vector<GcObject*>& roots);
    size_t live_count() const { return objects_.size(); }

private:
    void mark(GcObject* obj);

    std::vector<GcObject*> objects_;
};

} // namespace charcoal1
