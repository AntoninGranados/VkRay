#pragma once

#include <string>
#include <variant>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

enum class Interpolation { Step, Linear, Cubic, EaseIn, EaseOut, EaseInOut };

using KeyframeValue = std::variant<
    bool,
    int, glm::ivec2, glm::ivec3, glm::ivec4,
    float, glm::vec2, glm::vec3, glm::vec4,
    glm::quat,
    std::string
>;

struct Keyframe {
    int frame;
    KeyframeValue value;
    Interpolation interpolation = Interpolation::Linear;

    static KeyframeValue interpolate(const Keyframe& prev, const Keyframe& next, float t);

private:
    static float interpolationT(Interpolation interpolation, float t);
};
