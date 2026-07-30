#include "track.hpp"

void Track::setKeyframe(int frame, KeyframeValue value) {
    auto it = keyframes.find(frame);
    if (it != keyframes.end())
        it->second.value = std::move(value);
    else
        keyframes.emplace(frame, Keyframe{ .frame = frame, .value = std::move(value) });
}

void Track::setInterpolation(int frame, Interpolation interpolation) {
    auto it = keyframes.find(frame);
    if (it != keyframes.end())
        it->second.interpolation = interpolation;
}

void Track::removeKeyframe(int frame) {
    keyframes.erase(frame);
}

bool Track::hasKeyframe(int frame) const {
    return keyframes.count(frame) > 0;
}

const std::map<int, Keyframe>& Track::getKeyframes() const {
    return keyframes;
}

bool Track::isEmpty() const {
    return keyframes.empty();
}
