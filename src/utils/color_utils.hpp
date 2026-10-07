#pragma once

#include <glm/glm.hpp>

inline constexpr glm::vec3 kRec709Luminance{0.2126f, 0.7152f, 0.0722f};

inline float luminance(const glm::vec3& color) { return glm::dot(color, kRec709Luminance); }
