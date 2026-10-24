#pragma once

#include <iterator>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "core/animation/keyframe.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/fields/field.hpp"

class AnimationStore {
public:
    void capture(Field& field, int frame, Interpolation interp = Interpolation::Linear);
    void insert(Field& field, FieldValue value, int frame, Interpolation interp = Interpolation::Linear);
    bool has(Field& field, int frame) const;
    void remove(Field& field, int frame);
    void setInterpolation(Field& field, int frame, Interpolation interp);
    const std::map<int, Keyframe>& keyframes(Field& field) const;
    void remove(ecs::Registry& registry, ecs::Entity& e);
    std::unordered_set<const Field*> evaluate(float frame);
    void clear();
    bool isEmpty() const;

    template <typename T> T sampleAt(Field& field, float frame) const {
        const auto it = tracks.find(&field);
        if (it == tracks.end() || it->second.empty()) return field.get<T>();
        return sample<T>(it->second, frame);
    }

private:
    std::unordered_map<Field*, std::map<int, Keyframe>> tracks;

    template <typename T> static T sample(const std::map<int, Keyframe>& track, float frame) {
        if (frame <= float(track.begin()->first)) return track.begin()->second.value.get<T>();
        if (frame >= float(track.rbegin()->first)) return track.rbegin()->second.value.get<T>();
        const auto next = track.upper_bound(static_cast<int>(frame));
        const auto prev = std::prev(next);
        const float t = (frame - float(prev->first)) / float(next->first - prev->first);
        return Keyframe::interpolate<T>(prev->second, next->second, t);
    }
};
