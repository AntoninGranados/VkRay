#include "camera.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/registry.hpp"

#include <cmath>
#include <cstddef>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/core.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/scene/scene.hpp"

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler) {
    return glm::normalize(glm::quat(glm::radians(rotationEuler)) * glm::vec3(0.0f, 0.0f, -1.0f));
}

float effectiveFov(const ecs::Registry& registry, ecs::Entity camera) {
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
    const float fov = fovFromFocalLength(c.get<float>("focal_length") / sensorWidth);
    if (&registry != &Core::getScene().getRegistry() || Core::getRenderMode() != RenderMode::Preview || !Core::getScene().isUsingSceneCamera() || camera != Core::getScene().getCamera())
        return fov;
    return glm::degrees(2.0f * glm::atan(glm::tan(glm::radians(fov) * 0.5f) / 0.8f));
}

glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera) {
    const ecs::Component& t = registry.get(camera, ecs::Transform);
    const glm::vec3 position = t.get<glm::vec3>("position");
    return glm::lookAt(
        position,
        position + directionFromRotation(t.get<glm::vec3>("rotation")),
        glm::vec3(0, 1, 0)
    );
}

glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect) {
    return glm::perspective(
        glm::radians(effectiveFov(registry, camera)),
        aspect, 1e-4f, 1e4f
    );
}

float fovFromFocalLength(float normalizedFocalLength) {
    return 2.0f * glm::degrees(glm::atan(0.5f / normalizedFocalLength));
}

float focalLengthFromFov(float fovDegrees) {
    return 0.5f / glm::tan(glm::radians(fovDegrees) * 0.5f);
}

float lensRadiusFromFStop(float normalizedFocalLength, float fStop) {
    return fStop > 0.0f ? normalizedFocalLength / (2.0f * fStop) : 0.0f;
}

CameraUBO buildCameraUBO(ecs::Registry& registry, ecs::Entity camera, float aspect) {
    const float tanHFov = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);

    CameraUBO ubo{};
    ubo.U = aspect * tanHFov;
    ubo.V = tanHFov;
    ubo.motionOffset = registry.ctx().get<CameraMotionInfo>().motionOffset;

    {
        const ecs::Component& c = registry.get(camera, ecs::Camera);
        const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
        ubo.thinLens.lensRadius = lensRadiusFromFStop(c.get<float>("focal_length") / sensorWidth, c.get<float>("f_stop"));
        ubo.thinLens.focusDistance = c.get<float>("focal_distance");
    }

    CameraLensTable::pack(registry, camera, ubo);

    return ubo;
}

float blurFractionFromShutter(float seconds, float fps) {
    return seconds * fps;
}
