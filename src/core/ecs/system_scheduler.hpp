#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "./registry.hpp"

namespace ecs {

template <typename... Args>
class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&, Args...)>;

    void add(SystemFn fn) { systems.push_back(std::move(fn)); }
    void clear() { systems.clear(); }

    void run(Registry& registry, Args... args) {
        for (SystemFn& sys : systems) sys(registry, args...);
    }

private:
    std::vector<SystemFn> systems;
};

} // namespace ecs
