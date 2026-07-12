#pragma once

#include <glm/glm.hpp>

namespace ecs {

struct Quad {
    glm::vec3 u;
    glm::vec3 v;
    glm::vec3 normal;
};

} // namespace ecs
