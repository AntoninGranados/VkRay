#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "./registry.hpp"

namespace ecs {

class System {
public:
    explicit System(std::vector<const ComponentType*> reads = {}) : reads(std::move(reads)) {}
    virtual ~System() = default;

    virtual void run(Registry& registry) = 0;
    virtual bool forcesRun(Registry&) const { return false; }

    const std::vector<const ComponentType*>& getReads() const { return reads; }

private:
    std::vector<const ComponentType*> reads;
};

class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&)>;
    using Condition = std::function<bool(Registry&)>;

    void add(SystemFn fn) { systems.emplace_back(std::make_unique<FunctionSystem>(std::move(fn)), false); }

    void add(SystemFn fn, std::vector<const ComponentType*> reads, Condition runIf = {}) {
        systems.emplace_back(std::make_unique<FunctionSystem>(std::move(fn), std::move(reads), std::move(runIf)), true);
    }

    void add(std::unique_ptr<System> system) { systems.emplace_back(std::move(system), true); }

    void clear() { systems.clear(); }

    void invalidate() {
        for (Entry& entry : systems) entry.lastRun.reset();
    }

    void run(Registry& registry) {
        for (Entry& entry : systems) {
            if (!shouldRun(entry, registry)) continue;
            entry.system->run(registry);
            entry.lastRun = registry.getChangeTick();
        }
    }

private:
    class FunctionSystem : public System {
    public:
        explicit FunctionSystem(SystemFn fn, std::vector<const ComponentType*> reads = {}, Condition runIf = {})
            : System(std::move(reads)), fn(std::move(fn)), runIf(std::move(runIf)) {}

        void run(Registry& registry) override { fn(registry); }
        bool forcesRun(Registry& registry) const override { return runIf && runIf(registry); }

    private:
        SystemFn fn;
        Condition runIf;
    };

    struct Entry {
        Entry(std::unique_ptr<System> system, bool tracked) : system(std::move(system)), tracked(tracked) {}

        std::unique_ptr<System> system;
        bool tracked;
        std::optional<uint64_t> lastRun;
    };

    std::vector<Entry> systems;

    bool shouldRun(const Entry& entry, Registry& registry) const {
        if (!entry.tracked || !entry.lastRun) return true;
        if (entry.system->forcesRun(registry)) return true;
        for (const ComponentType* type : entry.system->getReads())
            if (registry.getChangeTick(*type) > *entry.lastRun) return true;
        return false;
    }
};

} // namespace ecs
