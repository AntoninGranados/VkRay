#pragma once

#include <map>

#include "core/animation/keyframe.hpp"

class Track {
public:
    void setKeyframe(int frame, FieldValue value, Interpolation interp = Interpolation::Linear);

    template<typename T>
    T sample(float frame) const {
        if (keyframes.empty()) return T{};
        if (frame <= float(keyframes.begin()->first)) return keyframes.begin()->second.getValue().get<T>();
        if (frame >= float(keyframes.rbegin()->first)) return keyframes.rbegin()->second.getValue().get<T>();
        const auto next = keyframes.upper_bound(static_cast<int>(frame));
        const auto prev = std::prev(next);
        const float t = (frame - float(prev->first)) / float(next->first - prev->first);
        return Keyframe::interpolate<T>(prev->second, next->second, t);
    }

    void setInterpolation(int frame, Interpolation interpolation);
    void erase(int frame);
    bool has(int frame) const;
    const std::map<int, Keyframe>& getKeyframes() const { return keyframes; }
    bool isEmpty() const { return keyframes.empty(); }

private:
    std::map<int, Keyframe> keyframes;
};
