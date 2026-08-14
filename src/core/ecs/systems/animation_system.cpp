#include "animation_system.hpp"

#include <random>

#include "core/animation/animation_store.hpp"
#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void animationSystem(Registry& registry) {
    static int prevFrame = -1;
    static std::mt19937 rng(std::random_device{}());

    const int currFrame = Core::getAnimation().getFrame();
    const bool frameChanged = currFrame != prevFrame;

    AnimationStore& store = Core::getScene().getAnimationStore();
    if (store.isEmpty()) return;

    if (frameChanged) {
        prevFrame = currFrame;
        store.evaluate(registry, float(currFrame));
        Core::getScene().update();
        Core::requestAccumulationRestart();
        return;
    }

    if (Core::getRenderMode() == RenderMode::Preview) return;

    const float shutterSpeed = Core::getScene().getCamera().getShutterSpeed();
    if (shutterSpeed <= 0.0f) return;

    std::uniform_real_distribution<float> dist(0.0f, shutterSpeed);
    const float t = static_cast<float>(currFrame) + dist(rng);
    store.evaluate(registry, t);
    Core::getScene().update();
}

} // namespace ecs
