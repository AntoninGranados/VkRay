#include "camera_system.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/camera.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry) {
    ::Camera& sceneCamera = Core::getScene().getCamera();
    if (!sceneCamera.hasPreviewCamera()) return;

    const ecs::Entity previewEnt = sceneCamera.getPreviewCamera();
    auto& cameras = registry.storage(Camera);
    auto& transforms = registry.storage(Transform);

    if (!cameras.has(previewEnt) || !transforms.has(previewEnt)) return;

    const Component& t = transforms.get(previewEnt);
    const Component& c = cameras.get(previewEnt);
    const glm::quat q = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
    const glm::vec3 pos = t.get<glm::vec3>("position");
    const float dist = glm::max(0.1f, glm::length(sceneCamera.getTarget() - sceneCamera.getPosition()));
    sceneCamera.setPosition(pos);
    sceneCamera.setTarget(pos + glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f)) * dist);
    sceneCamera.setFov(c.get<float>("fov"));
    sceneCamera.setAperture(c.get<float>("aperture"));
    sceneCamera.setFocusDepth(c.get<float>("focus_depth"));
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
        cameras.get(previewEnt).set<bool>("is_preview", false);
    }
    sceneCamera.clearPreviewCamera();
    sceneCamera.setAperture(0.0f);
    Core::requestAccumulationRestart();
}

} // namespace ecs
