#include "animation_system.hpp"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"

namespace ecs {

void evaluateAnimation(Registry& registry) {
    AnimationStore& store = *registry.ctx().get<AnimationStore*>();
    if (store.isEmpty()) return;

    store.evaluate(registry, Core::getAnimation().getSampleFrame());
    Core::markDirty();
}

void animationSystem(Registry& registry) {
    if (!Core::getAnimation().didFrameChange()) return;
    evaluateAnimation(registry);
}

} // namespace ecs
