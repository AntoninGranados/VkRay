#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/ecs/entity.hpp"
#include "core/render_structures.hpp"

namespace ecs { class Registry; }

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler);
float effectiveFov(const ecs::Registry& registry, ecs::Entity camera);

glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera);
glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect);
CameraUBO buildCameraUBO(ecs::Registry& registry, ecs::Entity camera, float aspect);

float fovFromFocalLength(float normalizedFocalLength);
float focalLengthFromFov(float fovDegrees);
float lensRadiusFromFStop(float normalizedFocalLength, float fStop);
float blurFractionFromShutter(float seconds, float fps);
