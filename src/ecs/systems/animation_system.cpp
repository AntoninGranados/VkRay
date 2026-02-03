#include "camera_system.hpp"

#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../animation_handler.hpp"

#include "../components.hpp"

#include <iostream>
namespace ecs {

void transformAnimationSystem(Registry& registry, AppContext& ctx) {
    static int prevFrame = 0;
    if (ctx.animation->isPaused() && prevFrame == ctx.animation->getFrame()) return;
    prevFrame = ctx.animation->getFrame();

    auto& transformAnims = registry.storage<ecs::TransformAnim>();
    auto& transforms = registry.storage<ecs::Transform>();

    for (const auto& e : transformAnims.entities()) {
        if (!transforms.has(e)) continue;

        auto& anim = transformAnims.get(e);
        if (anim.positionKeys.empty()) continue;

        auto& transform = transforms.get(e);

        int currFrame = ctx.animation->getFrame();
        if (currFrame < anim.positionKeys.front().frame) {
            transform.setPosition(anim.positionKeys.front().value);
            *ctx.restartRender = true;
            continue;
        }
        if (currFrame >= anim.positionKeys.back().frame) {
            transform.setPosition(anim.positionKeys.back().value);
            *ctx.restartRender = true;
            continue;
        }

        KeyVec3 prevKf = { 0, anim.positionKeys[0].value };
        KeyVec3 nextKf;
        for (const auto& kf : anim.positionKeys) {
            nextKf = kf;
            if (prevKf.frame <= currFrame && currFrame < nextKf.frame) break;
            prevKf = nextKf;
        }
        
        const float t = static_cast<float>(ctx.animation->getFrame() - prevKf.frame) / static_cast<float>(nextKf.frame - prevKf.frame);
        transform.setPosition(prevKf.value * (1 - t) + nextKf.value * t);
        *ctx.restartRender = true;
    }
}

} // namespace ecs
