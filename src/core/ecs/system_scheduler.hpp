#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "./registry.hpp"

namespace ecs {

class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&)>;

    void add(SystemFn fn) { systems.push_back(std::move(fn)); }
    void clear() { systems.clear(); }

    void run(Registry& registry) {
        for (SystemFn& sys : systems) sys(registry);
    }

private:
    std::vector<SystemFn> systems;
};

} // namespace ecs
