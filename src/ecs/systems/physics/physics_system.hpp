#pragma once

#include "../../registry.hpp"
#include "app/app_context.hpp"

namespace ecs {

void physicsSolver(Registry& registry, AppContext& ctx);
void bakePhysicsSimulation(Registry& registry, AppContext& ctx);
bool isPhysicsBakeInProgress();
int getPhysicsBakeCurrentFrame();
int getPhysicsBakeTotalFrames();
void physicsSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
