#include "keyframe.hpp"

#include <type_traits>

#include <glm/gtc/quaternion.hpp>

float Keyframe::interpolationT(Interpolation interpolation, float t) {
    switch (interpolation) {
        case Interpolation::Step:
        case Interpolation::Linear:    return t;
        case Interpolation::Cubic:     return t * t * (3.0f - 2.0f * t);
        case Interpolation::EaseIn:    return t * t * t;
        case Interpolation::EaseOut:   { const float u = 1.0f - t; return 1.0f - u * u * u; }
        case Interpolation::EaseInOut: { const float u = -2.0f * t + 2.0f; return t < 0.5f ? 4.0f * t * t * t : 1.0f - u * u * u / 2.0f; }
    }
    return t;
}

KeyframeValue Keyframe::interpolate(const Keyframe& prev, const Keyframe& next, float t) {
    if (prev.interpolation == Interpolation::Step) return prev.value;
    const float it = interpolationT(prev.interpolation, t);
    return std::visit([it](const auto& a, const auto& b) -> KeyframeValue {
        using A = std::decay_t<decltype(a)>;
        using B = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<A, B>) {
            if constexpr (std::is_same_v<A, float> || std::is_same_v<A, glm::vec2> || std::is_same_v<A, glm::vec3> || std::is_same_v<A, glm::vec4>)
                return glm::mix(a, b, it);
            else if constexpr (std::is_same_v<A, glm::quat>)
                return glm::slerp(a, b, it);
            else
                return KeyframeValue{a};
        } else {
            return KeyframeValue{a};
        }
    }, prev.value, next.value);
}
