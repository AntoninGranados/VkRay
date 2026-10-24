#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/entity.hpp"
#include "core/render_structures.hpp"

namespace ecs {
class Registry;
}

class Scene;

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler);

glm::vec2 applySensorFit(ecs::CameraSensorFit fit, float aspect, float sensorAxisValue);

struct CameraFrustum {
    glm::vec2 half;
    bool orthographic;
};

CameraFrustum computeFrustum(const ecs::Registry& registry, ecs::Entity camera, float aspect,
                             float frustumScale = 1.0f);

glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera);
glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect, float frustumScale);
CameraUBO buildCameraUBO(const Scene& scene, ecs::Entity camera, float aspect, float frustumScale);

float resolveFocusDistance(const ecs::Registry& registry, ecs::Entity camera);

float fovFromFocalLength(float normalizedFocalLength);
float focalLengthFromFov(float fovDegrees);
float lensRadiusFromFStop(float normalizedFocalLength, float fStop);
float blurFractionFromShutter(float seconds, float fps);
