#pragma once

#include <glm/glm.hpp>

namespace ecs {

struct Sphere {
    float radius = 1.0f;

    void setRadius(float newRadius) {
        radius = newRadius;
    }
};

} // namespace ecs
