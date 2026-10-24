#pragma once

class Scene;

namespace ecs {

struct PhysicsBakeState {
    bool isBaking = false;
    bool inProgress = false;
    int nextFrame = 0;
    int totalFrames = 0;
    int savedFrame = 0;
    bool wasPaused = true;
};

void physicsSolverSystem(Scene& scene);
void bakePhysicsSimulation(Scene& scene);
void physicsSystem(Scene& scene);

} // namespace ecs
