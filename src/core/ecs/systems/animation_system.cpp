#include "animation_system.hpp"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void evaluateAnimation(Scene& scene) {
    AnimationStore& store = scene.getAnimationStore();
    if (store.isEmpty()) return;

    scene.getRegistry().markChanged(store.evaluate(static_cast<float>(Core::getAnimation().getFrame())));
}

void animationSystem(Scene& scene) {
    if (!Core::getAnimation().didFrameChange()) return;
    evaluateAnimation(scene);
}

} // namespace ecs
