#include "animation_system.hpp"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void evaluateAnimation(Registry& registry) {
    AnimationStore& store = Core::getScene().getAnimationStore();
    if (store.isEmpty()) return;

    store.evaluate(registry, Core::getAnimation().getSampleFrame());
    Core::getScene().touch();
}

void animationSystem(Registry& registry) {
    if (!Core::getAnimation().didFrameChange()) return;
    evaluateAnimation(registry);
}

} // namespace ecs
