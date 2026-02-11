#pragma once

#include <glm/glm.hpp>

namespace ecs {

struct RigidBody {
    bool useGravity = true;
    float density = 50.0f;
    glm::vec3 linearVelocity { 0.0f, 0.0f, 0.0f };
    glm::vec3 angularVelocity { 0.0f, 0.0f, 0.0f };
};

} // namespace ecs
