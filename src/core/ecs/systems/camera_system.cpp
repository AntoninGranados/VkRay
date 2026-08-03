#include "camera_system.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry) {
    ::Camera& sceneCamera = Core::getScene().getCamera();
    auto& cameras = registry.storage(Camera);
    auto& transforms = registry.storage(Transform);
    auto& thinLensCameras = registry.storage(ThinLensCamera);

    if (sceneCamera.hasPreviewCamera()) {
        const ecs::Entity previewEnt = sceneCamera.getPreviewCamera();
        if (!cameras.has(previewEnt) || !transforms.has(previewEnt)) return;

        const Component& t = transforms.get(previewEnt);
        const Component& c = cameras.get(previewEnt);
        const glm::quat q = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
        const glm::vec3 pos = t.get<glm::vec3>("position");
        const float dist = glm::max(0.1f, glm::length(sceneCamera.getTarget() - sceneCamera.getPosition()));
        sceneCamera.setPosition(pos);
        sceneCamera.setTarget(pos + glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f)) * dist);
        sceneCamera.setFov(c.get<float>("fov"));
        sceneCamera.setShutterSpeed(c.get<float>("shutter_speed"));

        if (thinLensCameras.has(previewEnt)) {
            const Component& tl = thinLensCameras.get(previewEnt);
            sceneCamera.setAperture(tl.get<float>("aperture"));
            sceneCamera.setFocusDepth(tl.get<float>("focus_depth"));
        } else {
            sceneCamera.setAperture(0.0f);
            sceneCamera.setFocusDepth(10.0f);
        }
        return;
    }

    if (Core::getRenderMode() == RenderMode::Preview) {
        sceneCamera.setAperture(0.0f);
        return;
    }

    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        const Component& c = cameras.get(e);
        if (c.get<bool>("_is_preview")) continue;

        const Component& t = transforms.get(e);
        const glm::vec3 pos = t.get<glm::vec3>("position");
        const glm::vec3 dir = glm::normalize(
            glm::quat(glm::radians(t.get<glm::vec3>("rotation"))) * glm::vec3(0.0f, 0.0f, -1.0f));
        sceneCamera.setPosition(pos);
        sceneCamera.setFov(c.get<float>("fov"));
        sceneCamera.setShutterSpeed(c.get<float>("shutter_speed"));

        if (thinLensCameras.has(e)) {
            const Component& tl = thinLensCameras.get(e);
            const float focusDepth = glm::max(0.1f, tl.get<float>("focus_depth"));
            sceneCamera.setTarget(pos + dir * focusDepth);
            sceneCamera.setAperture(tl.get<float>("aperture"));
            sceneCamera.setFocusDepth(focusDepth);
        } else {
            sceneCamera.setTarget(pos + dir * 10.0f);
            sceneCamera.setAperture(0.0f);
            sceneCamera.setFocusDepth(10.0f);
        }
        break;
    }
}

void cameraPostUpdateSystem(Registry& registry) {
    ::Camera& sceneCamera = Core::getScene().getCamera();
    if (!sceneCamera.hasPreviewCamera()) return;

    const bool escapePressed = glfwGetKey(
        static_cast<GLFWwindow*>(Core::getPlatform().getNativeWindowHandle()), GLFW_KEY_ESCAPE
    );
    if (!escapePressed) return;

    const ecs::Entity previewEnt = sceneCamera.getPreviewCamera();
    auto& cameras = registry.storage(Camera);
    if (cameras.has(previewEnt)) {
        cameras.get(previewEnt).set<bool>("_is_preview", false);
    }
    sceneCamera.clearPreviewCamera();
    sceneCamera.setAperture(0.0f);
    Core::requestAccumulationRestart();
}

} // namespace ecs
