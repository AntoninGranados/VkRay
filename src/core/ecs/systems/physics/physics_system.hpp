#pragma once

#include "core/ecs/registry.hpp"

namespace ecs {

void physicsSolverSystem(Registry& registry);
void bakePhysicsSimulation(Registry& registry);
bool isPhysicsBakeInProgress();
int getPhysicsBakeCurrentFrame();
int getPhysicsBakeTotalFrames();
void physicsSystem(Registry& registry);

} // namespace ecs
