#pragma once

#include <type_traits>

#include "core/fields/field.hpp"

enum class Interpolation { Step, Linear, Cubic, EaseIn, EaseOut, EaseInOut };

struct Keyframe {
    FieldValue value;
    Interpolation interpolation = Interpolation::Linear;

    template <typename T> static T interpolate(const Keyframe& prev, const Keyframe& next, float t);

private:
    static float interpolationT(Interpolation interpolation, float t);
};

template <typename T> T Keyframe::interpolate(const Keyframe& prev, const Keyframe& next, float t) {
    if (prev.interpolation == Interpolation::Step) return prev.value.get<T>();
    const float it = interpolationT(prev.interpolation, t);
    if constexpr (std::is_same_v<T, glm::quat>)
        return glm::slerp(prev.value.get<glm::quat>(), next.value.get<glm::quat>(), it);
    else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> ||
                       std::is_same_v<T, glm::vec4>)
        return glm::mix(prev.value.get<T>(), next.value.get<T>(), it);
    else
        return prev.value.get<T>();
}
