#pragma once

#include "./registry.hpp"
#include "../app_context.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace ecs {

class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&, AppContext&)>;

    void add(SystemFn fn) { systems.push_back(std::move(fn)); }
    void clear() { systems.clear(); }

    void run(Registry& registry, AppContext& ctx) {
        for (SystemFn& sys : systems)
            sys(registry, ctx);
    }

private:
    std::vector<SystemFn> systems;
};

} // namespace ecs
