#pragma once

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/entity.hpp"
#include "core/render_structures.hpp"

namespace ecs { class Registry; }

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler);

glm::vec2 applySensorFit(ecs::CameraSensorFit fit, float aspect, float sensorAxisValue);

struct CameraFrustum {
    glm::vec2 half;
    bool orthographic;
};

using FrustumHook = std::function<float(const ecs::Registry&, ecs::Entity, float aspect, float trueValue)>;
void setFrustumHook(FrustumHook hook);

CameraFrustum computeFrustum(const ecs::Registry& registry, ecs::Entity camera, float aspect, bool applyHook = true);

glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera);
glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect);
CameraUBO buildCameraUBO(ecs::Registry& registry, ecs::Entity camera, float aspect);

float resolveFocusDistance(const ecs::Registry& registry, ecs::Entity camera);

float fovFromFocalLength(float normalizedFocalLength);
float focalLengthFromFov(float fovDegrees);
float lensRadiusFromFStop(float normalizedFocalLength, float fStop);
float blurFractionFromShutter(float seconds, float fps);
