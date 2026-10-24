#include "camera.hpp"

#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/registry.hpp"
#include "core/scene/scene.hpp"

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler) {
    return glm::normalize(glm::quat(glm::radians(rotationEuler)) * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec2 applySensorFit(ecs::CameraSensorFit fit, float aspect, float sensorAxisValue) {
    const bool horizontal =
        fit == ecs::CameraSensorFit::Horizontal || (fit == ecs::CameraSensorFit::Auto && aspect >= 1.0f);
    return horizontal ? glm::vec2(sensorAxisValue, sensorAxisValue / aspect)
                      : glm::vec2(sensorAxisValue * aspect, sensorAxisValue);
}

CameraFrustum computeFrustum(const ecs::Registry& registry, ecs::Entity camera, float aspect, float frustumScale) {
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const ecs::CameraSensorFit fit = static_cast<ecs::CameraSensorFit>(c.get<int>("sensor_fit"));
    const bool orthographic =
        static_cast<ecs::CameraProjection>(c.get<int>("projection")) == ecs::CameraProjection::Orthographic;

    const float trueValue =
        orthographic
            ? c.get<float>("sensor_width") / 1000.0f * 0.5f
            : glm::tan(glm::radians(fovFromFocalLength(c.get<float>("focal_length") / c.get<float>("sensor_width"))) *
                       0.5f);

    return {applySensorFit(fit, aspect, trueValue * frustumScale), orthographic};
}

glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera) {
    const ecs::Component& t = registry.get(camera, ecs::Transform);
    const glm::vec3 position = t.get<glm::vec3>("position");
    return glm::lookAt(position, position + directionFromRotation(t.get<glm::vec3>("rotation")), glm::vec3(0, 1, 0));
}

glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect, float frustumScale) {
    const CameraFrustum f = computeFrustum(registry, camera, aspect, frustumScale);
    if (f.orthographic) return glm::orthoRH_ZO(-f.half.x, f.half.x, -f.half.y, f.half.y, 1e4f, 1e-4f);
    const float fovY = 2.0f * glm::atan(f.half.y);
    return glm::perspectiveRH_ZO(fovY, aspect, 1e4f, 1e-4f);
}

float fovFromFocalLength(float normalizedFocalLength) {
    return 2.0f * glm::degrees(glm::atan(0.5f / normalizedFocalLength));
}

float focalLengthFromFov(float fovDegrees) { return 0.5f / glm::tan(glm::radians(fovDegrees) * 0.5f); }

float lensRadiusFromFStop(float normalizedFocalLength, float fStop) {
    return fStop > 0.0f ? normalizedFocalLength / (2.0f * fStop) : 0.0f;
}

float resolveFocusDistance(const ecs::Registry& registry, ecs::Entity camera) {
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const ecs::Entity focusTarget = c.get<ecs::Entity>("focus_target");
    if (focusTarget == ecs::Entity{} || !registry.has(focusTarget, ecs::Transform))
        return c.get<float>("focal_distance");

    const ecs::Component& camTransform = registry.get(camera, ecs::Transform);
    const glm::vec3 forward = directionFromRotation(camTransform.get<glm::vec3>("rotation"));
    const glm::vec3 offset =
        registry.get(focusTarget, ecs::Transform).get<glm::vec3>("position") - camTransform.get<glm::vec3>("position");
    return glm::max(glm::dot(offset, forward), 0.01f);
}

CameraUBO buildCameraUBO(const Scene& scene, ecs::Entity camera, float aspect, float frustumScale) {
    const ecs::Registry& registry = scene.getRegistry();
    const ecs::Component& c = registry.get(camera, ecs::Camera);
    const CameraFrustum f = computeFrustum(registry, camera, aspect, frustumScale);

    CameraUBO ubo{};
    ubo.projection = std::to_underlying(static_cast<ecs::CameraProjection>(c.get<int>("projection")));
    ubo.motionOffset = scene.getPackingInfo().cameraMotionOffset;
    ubo.thinLens.focusDistance = resolveFocusDistance(registry, camera);
    ubo.U = f.half.x;
    ubo.V = f.half.y;
    ubo.thinLens.lensRadius =
        f.orthographic
            ? 0.0f
            : lensRadiusFromFStop(c.get<float>("focal_length") / c.get<float>("sensor_width"), c.get<float>("f_stop"));

    const LensPluginInfo& lensInfo = scene.getPackingInfo().lens;
    ubo.lensSlot = lensInfo.slot;
    ubo.lensParamsBase = lensInfo.paramsBase;

    return ubo;
}

float blurFractionFromShutter(float seconds, float fps) { return seconds * fps; }
