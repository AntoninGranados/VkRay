#pragma once

#include <glm/glm.hpp>

#include "core/ecs/components/component.hpp"

glm::mat4 composeTransform(const ecs::Component& transform);
void applyTransform(ecs::Component& transform, const glm::mat4& matrix);
