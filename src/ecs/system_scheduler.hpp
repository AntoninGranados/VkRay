#pragma once

#include "./registry.hpp"
#include "app/app_context.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace ecs {

template <typename... ExtraArgs>
class SystemScheduler {
public:
    using SystemFn = std::function<void(Registry&, AppContext&, ExtraArgs...)>;

    void add(SystemFn fn) { systems.push_back(std::move(fn)); }
    void clear() { systems.clear(); }

    void run(Registry& registry, AppContext& ctx, ExtraArgs... extraArgs) {
        for (SystemFn& sys : systems)
            sys(registry, ctx, extraArgs...);
    }

private:
    std::vector<SystemFn> systems;
};

} // namespace ecs
