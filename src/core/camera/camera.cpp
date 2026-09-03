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

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler) {
    return glm::normalize(glm::quat(glm::radians(rotationEuler)) * glm::vec3(0.0f, 0.0f, -1.0f));
}

float effectiveFov(const ecs::Registry& registry, ecs::Entity camera) {
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const float fov = c.get<float>("fov");
    if (&registry != &Core::getScene().getRegistry() || Core::getRenderMode() != RenderMode::Preview || !Core::getScene().isUsingSceneCamera() || camera != Core::getScene().getCamera())
        return fov;
    return glm::degrees(2.0f * glm::atan(glm::tan(glm::radians(fov) * 0.5f) / 0.8f));
}

std::optional<TiltShiftState> getTiltShiftState(const ecs::Registry& registry, ecs::Entity camera) {
    if (!registry.has(camera, ecs::TiltShiftLens)) return std::nullopt;

    const ecs::Component& t = registry.get(camera, ecs::Transform);
    const ecs::Component& tl = registry.get(camera, ecs::TiltShiftLens);

    const glm::vec3 worldNormal = glm::normalize(
        glm::quat(glm::radians(tl.get<glm::vec3>("plane_rotation"))) * glm::vec3(0.0f, 0.0f, 1.0f)
    );

    const glm::vec3 camDir = directionFromRotation(t.get<glm::vec3>("rotation"));
    const glm::vec3 camRight = glm::normalize(glm::cross(camDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(camRight, camDir);
    const glm::vec3 diff = tl.get<glm::vec3>("plane_position") - t.get<glm::vec3>("position");

    const glm::vec3 center = glm::vec3(
        glm::dot(diff, camRight),
        glm::dot(diff, camUp),
        glm::dot(diff, camDir)
    );
    const glm::vec3 normal = glm::normalize(glm::vec3(
        glm::dot(worldNormal, camRight),
        glm::dot(worldNormal, camUp),
        glm::dot(worldNormal, camDir))
    );

    const glm::vec3 arbUp = std::abs(glm::dot(normal, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f
        ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(normal, arbUp));
    const glm::vec3 upOnPlane = glm::cross(normal, right);

    TiltShiftState state;
    state.focusA = center + right;
    state.focusB = center - right;
    state.focusC = center + upOnPlane;

    return state;
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

CameraUBO buildCameraUBO(const ecs::Registry& registry, ecs::Entity camera, float aspect) {
    const ecs::Component& t = registry.get(camera, ecs::Transform);

    const glm::vec3 dir = directionFromRotation(t.get<glm::vec3>("rotation"));
    const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(right, dir);
    const float tanHFov = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);

    CameraUBO ubo{};
    ubo.eye = t.get<glm::vec3>("position");
    ubo.U = right * aspect * tanHFov;
    ubo.V = camUp * tanHFov;
    ubo.W = dir;

    ubo.thinLens.lensRadius = 0.0f;
    ubo.thinLens.focusDistance = 10.0f;
    if (registry.has(camera, ecs::ThinLens)) {
        const ecs::Component& tl = registry.get(camera, ecs::ThinLens);
        const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
        ubo.thinLens.lensRadius = lensRadiusFromFStop(tl.get<float>("focal_length") / sensorWidth, tl.get<float>("f_stop"));
        ubo.thinLens.focusDistance = tl.get<float>("focal_distance");
    }

    const auto ts = getTiltShiftState(registry, camera);
    ubo.tiltShift.enabled = ts.has_value();
    if (ts.has_value()) {
        ubo.tiltShift.focusA = ts->focusA;
        ubo.tiltShift.focusB = ts->focusB;
        ubo.tiltShift.focusC = ts->focusC;
    }

    return ubo;
}

float blurFractionFromShutter(float seconds, float fps) {
    return seconds * fps;
}
