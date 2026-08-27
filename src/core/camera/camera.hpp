#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/ecs/entity.hpp"

namespace ecs { class Registry; }

struct TiltShiftState {
    glm::vec3 focusA  = { -1.0f, 0.0f, 5.0f };
    glm::vec3 focusB  = {  1.0f, 0.0f, 5.0f };
    glm::vec3 focusC  = {  0.0f, 1.0f, 5.0f };
};

glm::vec3 directionFromRotation(const glm::vec3& rotationEuler);
float effectiveFov(const ecs::Registry& registry, ecs::Entity camera);

std::optional<TiltShiftState> getTiltShiftState(const ecs::Registry& registry, ecs::Entity camera);
glm::mat4 getView(const ecs::Registry& registry, ecs::Entity camera);
glm::mat4 getProjection(const ecs::Registry& registry, ecs::Entity camera, float aspect);

float fovFromFocalLength(float normalizedFocalLength);
float focalLengthFromFov(float fovDegrees);
float lensRadiusFromFStop(float normalizedFocalLength, float fStop);
float blurFractionFromShutter(float seconds, float fps);
