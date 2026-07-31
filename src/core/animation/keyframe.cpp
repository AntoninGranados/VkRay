#include "keyframe.hpp"

float Keyframe::interpolationT(Interpolation interpolation, float t) {
    switch (interpolation) {
        case Interpolation::Step:
        case Interpolation::Linear: return t;
        case Interpolation::Cubic: return t * t * (3.0f - 2.0f * t);
        case Interpolation::EaseIn: return t * t * t;
        case Interpolation::EaseOut: { const float u = 1.0f - t; return 1.0f - u * u * u; }
        case Interpolation::EaseInOut: { const float u = -2.0f * t + 2.0f; return t < 0.5f ? 4.0f * t * t * t : 1.0f - u * u * u / 2.0f; }
    }
    return t;
}
