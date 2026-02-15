#pragma once

#include "../../registry.hpp"
#include "../../../app_context.hpp"

namespace ecs {

void physicsSolver(Registry& registry, AppContext& ctx);
void bakePhysicsSimulation(Registry& registry, AppContext& ctx);
void physicsSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
