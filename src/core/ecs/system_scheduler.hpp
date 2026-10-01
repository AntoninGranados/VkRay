#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "./registry.hpp"

namespace ecs {

class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&)>;
    using Condition = std::function<bool(Registry&)>;

    void add(SystemFn fn) { systems.push_back({.fn = std::move(fn)}); }

    void add(SystemFn fn, std::vector<const ComponentType*> reads, Condition runIf = {}) {
        systems.push_back({.fn = std::move(fn), .reads = std::move(reads), .runIf = std::move(runIf), .tracked = true});
    }

    void clear() { systems.clear(); }

    void invalidate() {
        for (System& system : systems) system.lastRun.reset();
    }

    void run(Registry& registry) {
        for (System& system : systems) {
            if (!shouldRun(system, registry)) continue;
            system.fn(registry);
            system.lastRun = registry.getChangeTick();
        }
    }

private:
    struct System {
        SystemFn fn;
        std::vector<const ComponentType*> reads;
        Condition runIf;
        bool tracked = false;
        std::optional<uint64_t> lastRun;
    };

    std::vector<System> systems;

    bool shouldRun(const System& system, Registry& registry) const {
        if (!system.tracked || !system.lastRun) return true;
        if (system.runIf && system.runIf(registry)) return true;
        for (const ComponentType* type : system.reads)
            if (registry.getChangeTick(*type) > *system.lastRun) return true;
        return false;
    }
};

} // namespace ecs
