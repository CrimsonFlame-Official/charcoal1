// Charcoal1 GC implementation.
#include "gc.h"

namespace charcoal1 {

GcHeap::~GcHeap() {
    for (auto* o : objects_) delete o;
}

void GcHeap::mark(GcObject* obj) {
    if (!obj || obj->marked) return;
    obj->marked = true;
    obj->trace(*this);
}

void GcHeap::collect(const std::vector<GcObject*>& roots) {
    // Phase 1: mark from roots.
    for (auto* r : roots) mark(r);

    // Phase 2: sweep unreachable; reset marks for the next cycle.
    std::vector<GcObject*> live;
    live.reserve(objects_.size());
    for (auto* obj : objects_) {
        if (obj->marked) {
            obj->marked = false;
            live.push_back(obj);
        } else {
            delete obj;
        }
    }
    objects_.swap(live);
}

} // namespace charcoal1
