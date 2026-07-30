#pragma once

#include <map>

#include "core/animation/keyframe.hpp"

class Track {
public:
    void setKeyframe(int frame, KeyframeValue value);
    void setInterpolation(int frame, Interpolation interpolation);
    void removeKeyframe(int frame);
    bool hasKeyframe(int frame) const;
    const std::map<int, Keyframe>& getKeyframes() const;
    bool isEmpty() const;

private:
    std::map<int, Keyframe> keyframes;
};
