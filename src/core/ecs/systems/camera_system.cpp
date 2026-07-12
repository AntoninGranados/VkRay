#include "camera_system.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "app_context.hpp"
#include "core/camera.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry, AppContext& ctx) {
    auto& cameras = registry.storage<ecs::CameraObject>();
    auto& transforms = registry.storage<ecs::Transform>();
    constexpr float previewViewportScale = 0.8f;
    
    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        
        ecs::CameraObject& c = cameras.get(e);
        if (ctx.renderMode != RenderMode::Preview && !c.isPreview) {
            c.setPreview(true);
            c.setPreviewJustSet(true);
            *ctx.restartRender = true;
        }

        if (!c.isPreview) continue;

        ecs::Transform& t = transforms.get(e);
        Camera& camera = *ctx.camera;

        const float dist = glm::max(0.1f, glm::length(camera.getTarget() - camera.getPosition()));
        const glm::vec3 dir = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));

        camera.setPosition(t.position);
        camera.setTarget(t.position + dir * dist);
        float fov = c.fov;
        if (ctx.renderMode == RenderMode::Preview) {
            const float baseFovRad = glm::radians(c.fov);
            const float previewFovRad = 2.0f * atanf(tanf(baseFovRad * 0.5f) / previewViewportScale);
            fov = glm::degrees(previewFovRad);
        }
        camera.setFov(fov);
        camera.setAperture(c.aperture);
        camera.setFocusDepth(c.focusDepth);
        break;
    }
}

void cameraPostUpdateSystem(Registry& registry, AppContext& ctx) {
    auto& cameras = registry.storage<ecs::CameraObject>();
    auto& transforms = registry.storage<ecs::Transform>();
    bool escapePressed = glfwGetKey(static_cast<GLFWwindow*>(ctx.platform->getNativeWindowHandle()), GLFW_KEY_ESCAPE);
    Camera& camera = *ctx.camera;
    constexpr float previewViewportScale = 0.8f;

    for (const auto& e : cameras.entities()) {
        ecs::CameraObject& c = cameras.get(e);
        if (!c.isPreview) continue;

        if (escapePressed) {
            c.setPreview(false);
            *ctx.restartRender = true;
            ctx.camera->setAperture(0.0f);
            continue;
        }

        if (c.previewJustSet) {
            c.previewJustSet = false;
            continue;
        }

        if (c.updated) {
            float fov = c.fov;
            if (ctx.renderMode == RenderMode::Preview) {
                const float baseFovRad = glm::radians(c.fov);
                const float previewFovRad = 2.0f * atanf(tanf(baseFovRad * 0.5f) / previewViewportScale);
                fov = glm::degrees(previewFovRad);
            }
            camera.setFov(fov);
            camera.setAperture(c.aperture);
            camera.setFocusDepth(c.focusDepth);
            c.updated = false;
            continue;
        }

        if (!transforms.has(e)) continue;
        ecs::Transform& t = transforms.get(e);

        if (t.updated) {
            const float dist = glm::max(0.1f, glm::length(camera.getTarget() - camera.getPosition()));
            const glm::vec3 dir = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
            camera.setPosition(t.position);
            camera.setTarget(t.position + dir * dist);
            t.updated = false;
            continue;
        }

        float fov = camera.getFov();
        if (ctx.renderMode == RenderMode::Preview) {
            const float previewFovRad = glm::radians(camera.getFov());
            const float baseFovRad = 2.0f * atanf(tanf(previewFovRad * 0.5f) * previewViewportScale);
            fov = glm::degrees(baseFovRad);
        }
        c.setFov(fov);
        c.setAperture(camera.getAperture());
        c.setFocusDepth(camera.getFocusDepth());
        t.setPosition(camera.getPosition());
        t.setRotation(glm::quatLookAt(glm::normalize(camera.getDirection()), camera.getUp()));
    }
}

} // namespace ecs
