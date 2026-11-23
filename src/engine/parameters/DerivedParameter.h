#ifndef DERIVED_PARAMETER_H
#define DERIVED_PARAMETER_H

#include "ofEvent.h"
#include "ofParameter.h"

namespace Engine::Parameters {

template <typename T>
class DerivedParameter {
public:
    DerivedParameter() = default;
    DerivedParameter(const DerivedParameter &) = delete;
    DerivedParameter &operator=(const DerivedParameter &) = delete;

    ofEvent<void> invalidated;

    /**
     * @brief	Binds the derived parameter to one or more OpenFrameworks parameters.
     * @param	fn	    Recompute function.
     * @param	deps	All OF parameters to bind to.
     */
    template <typename F, typename... Deps>
    void bind(F fn, Deps &...deps) {
        compute = [fn = std::move(fn), &deps...]() -> T { return fn(valueOf(deps)...); };
        listeners.unsubscribeAll();
        (listen(deps), ...);
        markDirty();
    }

    const T &get() {
        if (dirty) {
            value = compute();
            dirty = false;
        }
        return value;
    }

    void invalidate() {
        markDirty();
    }

private:
    std::function<T()> compute;
    ofEventListeners listeners;
    T value{};
    bool dirty = true;

    template <typename P>
    static const P &valueOf(ofParameter<P> &p) {
        return p.get();
    }

    template <typename U>
    static const U &valueOf(DerivedParameter<U> &d) {
        return d.get();
    }

    template <typename P>
    void listen(ofParameter<P> &p) {
        listeners.push(p.newListener([this](P &) { markDirty(); }));
    }

    template <typename U>
    void listen(DerivedParameter<U> &d) {
        listeners.push(d.invalidated.newListener([this]() { markDirty(); }));
    }

    void markDirty() {
        dirty = true;
        invalidated.notify(this);
    }
};

} // namespace Engine::Parameters

#endif
