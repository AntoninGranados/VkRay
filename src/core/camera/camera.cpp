#include "camera.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/registry.hpp"

#include <cmath>
#include <cstddef>
#include <utility>

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
    const float fov = fovFromFocalLength(c.get<float>("focal_length") / c.get<float>("sensor_width"));
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
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    if (static_cast<ecs::CameraProjection>(c.get<int>("projection")) == ecs::CameraProjection::Orthographic) {
        const float halfWidth = c.get<float>("sensor_width") / 1000.0f * 0.5f;
        const float halfHeight = halfWidth / aspect;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 1e-4f, 1e4f);
    }
    const float tanHalfFovH = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);
    const float fovY = 2.0f * glm::atan(tanHalfFovH / aspect);
    return glm::perspective(fovY, aspect, 1e-4f, 1e4f);
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
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const ecs::CameraProjection projection = static_cast<ecs::CameraProjection>(c.get<int>("projection"));

    CameraUBO ubo{};
    ubo.projection = std::to_underlying(projection);
    ubo.motionOffset = registry.ctx().get<CameraMotionInfo>().motionOffset;
    ubo.thinLens.focusDistance = c.get<float>("focal_distance");

    if (projection == ecs::CameraProjection::Orthographic) {
        const float halfWidth = c.get<float>("sensor_width") / 1000.0f * 0.5f;
        ubo.U = halfWidth;
        ubo.V = halfWidth / aspect;
        ubo.thinLens.lensRadius = 0.0f;
    } else {
        const float tanHalfFovH = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);
        ubo.U = tanHalfFovH;
        ubo.V = tanHalfFovH / aspect;
        ubo.thinLens.lensRadius = lensRadiusFromFStop(c.get<float>("focal_length") / c.get<float>("sensor_width"), c.get<float>("f_stop"));
    }

    CameraLensTable::pack(registry, camera, ubo);

    return ubo;
}

float blurFractionFromShutter(float seconds, float fps) {
    return seconds * fps;
}
