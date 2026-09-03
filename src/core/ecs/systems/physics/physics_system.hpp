#pragma once

#include "core/ecs/registry.hpp"

namespace ecs {

struct PhysicsBakeState {
    bool isBaking    = false;
    bool inProgress  = false;
    int  nextFrame   = 0;
    int  totalFrames = 0;
    int  savedFrame  = 0;
    bool wasPaused   = true;
};

void physicsSolverSystem(Registry& registry);
void bakePhysicsSimulation(Registry& registry);
bool isPhysicsBakeInProgress(const Registry& registry);
int getPhysicsBakeCurrentFrame(const Registry& registry);
int getPhysicsBakeTotalFrames(const Registry& registry);
void physicsSystem(Registry& registry);

} // namespace ecs
