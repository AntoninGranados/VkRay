#include "track.hpp"

void Track::setKeyframe(int frame, FieldValue value, Interpolation interp) {
    auto it = keyframes.find(frame);
    if (it != keyframes.end()) {
        it->second.setValue(std::move(value));
        it->second.setInterpolation(interp);
    } else {
        keyframes.emplace(frame, Keyframe(frame, std::move(value), interp));
    }
}

void Track::setInterpolation(int frame, Interpolation interpolation) {
    auto it = keyframes.find(frame);
    if (it != keyframes.end())
        it->second.setInterpolation(interpolation);
}

void Track::erase(int frame) {
    keyframes.erase(frame);
}

bool Track::has(int frame) const {
    return keyframes.contains(frame);
}
