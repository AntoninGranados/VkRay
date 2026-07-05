#include "animation_system.hpp"

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "app/animation_handler.hpp"
#include "scene/scene.hpp"

#include "../components.hpp"

namespace ecs {

void transformAnimationSystem(Registry& registry, AppContext& ctx) {
    static int prevFrame = 0;
    if (ctx.animation->isPaused() && prevFrame == ctx.animation->getFrame() && !*ctx.restartRender) return;
    prevFrame = ctx.animation->getFrame();

    auto& transformAnims = registry.storage<ecs::TransformAnim>();
    auto& transforms = registry.storage<ecs::Transform>();

    for (const auto& e : transformAnims.entities()) {
        if (!transforms.has(e)) continue;
        auto& transform = transforms.get(e);
        const int currFrame = ctx.animation->getFrame();

        auto& anim = transformAnims.get(e);
        bool changed = false;

        if (!anim.positionKeys.empty()) {
            if (currFrame < anim.positionKeys.front().frame) {
                transform.setPosition(anim.positionKeys.front().value);
                changed = true;
            } else if (currFrame >= anim.positionKeys.back().frame) {
                transform.setPosition(anim.positionKeys.back().value);
                changed = true;
            } else {
                KeyVec3 prevKf = { 0, anim.positionKeys[0].value };
                KeyVec3 nextKf = anim.positionKeys[0];
                for (const auto& kf : anim.positionKeys) {
                    nextKf = kf;
                    if (prevKf.frame <= currFrame && currFrame < nextKf.frame) break;
                    prevKf = nextKf;
                }
                
                const float t = static_cast<float>(currFrame - prevKf.frame) / static_cast<float>(nextKf.frame - prevKf.frame);
                transform.setPosition(prevKf.value * (1.0f - t) + nextKf.value * t);
                changed = true;
            }
        }
        
        if (!anim.rotationKeys.empty()) {
            if (currFrame < anim.rotationKeys.front().frame) {
                transform.setRotation(anim.rotationKeys.front().value);
                changed = true;
            } else if (currFrame >= anim.rotationKeys.back().frame) {
                transform.setRotation(anim.rotationKeys.back().value);
                changed = true;
            } else {
                KeyQuat prevKf = { 0, anim.rotationKeys[0].value };
                KeyQuat nextKf = anim.rotationKeys[0];
                for (const auto& kf : anim.rotationKeys) {
                    nextKf = kf;
                    if (prevKf.frame <= currFrame && currFrame < nextKf.frame) break;
                    prevKf = nextKf;
                }
                
                const float t = static_cast<float>(currFrame - prevKf.frame) / static_cast<float>(nextKf.frame - prevKf.frame);
                const glm::quat q0 = glm::normalize(prevKf.value);
                glm::quat q1 = glm::normalize(nextKf.value);
                if (glm::dot(q0, q1) < 0.0f) q1 = -q1; // Shortest-path interpolation.
                transform.setRotation(glm::slerp(q0, q1, t));
                changed = true;
            }
        }

        if (!anim.scaleKeys.empty()) {
            if (currFrame < anim.scaleKeys.front().frame) {
                transform.setScale(anim.scaleKeys.front().value);
                changed = true;
            } else if (currFrame >= anim.scaleKeys.back().frame) {
                transform.setScale(anim.scaleKeys.back().value);
                changed = true;
            } else {
                KeyVec3 prevKf = { 0, anim.scaleKeys[0].value };
                KeyVec3 nextKf = anim.scaleKeys[0];
                for (const auto& kf : anim.scaleKeys) {
                    nextKf = kf;
                    if (prevKf.frame <= currFrame && currFrame < nextKf.frame) break;
                    prevKf = nextKf;
                }

                const float t = static_cast<float>(currFrame - prevKf.frame) / static_cast<float>(nextKf.frame - prevKf.frame);
                transform.setScale(prevKf.value * (1.0f - t) + nextKf.value * t);
                changed = true;
            }
        }

        if (changed) {
            *ctx.restartRender = true;
        }
    }
}

void materialAnimationSystem(Registry& registry, AppContext& ctx) {
    static int prevFrame = -1;
    const int currFrame = ctx.animation->getFrame();
    if (currFrame == prevFrame && !*ctx.restartRender) return;
    const bool frameChanged = (currFrame != prevFrame);
    prevFrame = currFrame;

    auto& materialAnims = registry.storage<ecs::MaterialAnim>();
    auto& materialRefs  = registry.storage<ecs::MaterialRef>();
    auto& materials     = ctx.scene->getMaterials();

    for (const auto& e : materialAnims.entities()) {
        if (!materialRefs.has(e)) continue;

        auto&     anim   = materialAnims.get(e);
        const int handle = materialRefs.get(e).handle;

        if (handle < 0 || handle >= static_cast<int>(materials.size())) continue;

        const KeyMaterial kf = anim.getKeyframe(currFrame);
        Material& mat        = materials[handle];

        mat.roughness        = kf.roughness;
        mat.metalness        = kf.metalness;
        mat.ior              = kf.ior;
        mat.transmission     = kf.transmission;
        mat.emissionStrength = kf.emissionStrength;
    }

    if (frameChanged) *ctx.restartRender = true;
}

} // namespace ecs
