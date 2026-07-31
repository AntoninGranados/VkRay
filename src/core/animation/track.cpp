#include "track.hpp"

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
