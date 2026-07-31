#pragma once

#include <type_traits>

#include "core/field.hpp"

enum class Interpolation { Step, Linear, Cubic, EaseIn, EaseOut, EaseInOut };

struct Keyframe {
    Keyframe(int frame, FieldValue value, Interpolation interpolation = Interpolation::Linear)
        : frameNumber(frame), fieldValue(std::move(value)), interpolationMode(interpolation) {}

    int getFrame() const { return frameNumber; }
    const FieldValue& getValue() const { return fieldValue; }
    Interpolation getInterpolation() const { return interpolationMode; }
    void setInterpolation(Interpolation interp) { interpolationMode = interp; }
    void setValue(FieldValue v) { fieldValue = std::move(v); }

    template<typename T>
    static T interpolate(const Keyframe& prev, const Keyframe& next, float t);

private:
    int frameNumber;
    FieldValue fieldValue;
    Interpolation interpolationMode = Interpolation::Linear;

    static float interpolationT(Interpolation interpolation, float t);
};

template<typename T>
T Keyframe::interpolate(const Keyframe& prev, const Keyframe& next, float t) {
    if (prev.interpolationMode == Interpolation::Step) return prev.fieldValue.get<T>();
    const float it = interpolationT(prev.interpolationMode, t);
    if constexpr (std::is_same_v<T, glm::quat>)
        return glm::slerp(prev.fieldValue.get<glm::quat>(), next.fieldValue.get<glm::quat>(), it);
    else if constexpr (std::is_same_v<T, float>    ||
                       std::is_same_v<T, glm::vec2> ||
                       std::is_same_v<T, glm::vec3> ||
                       std::is_same_v<T, glm::vec4>)
        return glm::mix(prev.fieldValue.get<T>(), next.fieldValue.get<T>(), it);
    else
        return prev.fieldValue.get<T>();
}
