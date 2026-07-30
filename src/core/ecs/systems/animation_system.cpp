#include "animation_system.hpp"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void animationSystem(Registry& registry) {
    static int prevFrame = -1;
    const int currFrame = Core::getAnimation().getFrame();
    if (currFrame == prevFrame && !Core::isAccumulationRestartPending()) return;
    prevFrame = currFrame;

    AnimationStore& store = Core::getScene().getAnimationStore();
    if (store.isEmpty()) return;
    store.evaluate(registry, float(currFrame));
    store.evaluate(Core::getScene().getMaterials(), float(currFrame));
    Core::requestAccumulationRestart();
}

} // namespace ecs
