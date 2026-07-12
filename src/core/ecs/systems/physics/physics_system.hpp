#pragma once

#include "../../registry.hpp"
#include "app_context.hpp"

class AnimationHandler;

namespace ecs {

void physicsSolverSystem(Registry& registry, AppContext& ctx);
void bakePhysicsSimulation(Registry& registry, AnimationHandler* animation, bool& restartRender);
bool isPhysicsBakeInProgress();
int getPhysicsBakeCurrentFrame();
int getPhysicsBakeTotalFrames();
void physicsSystem(Registry& registry, AppContext& ctx);

} // namespace ecs
