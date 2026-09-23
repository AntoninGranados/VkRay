#pragma once

#include <map>
#include <unordered_map>

#include "core/animation/keyframe.hpp"
#include "core/animation/track.hpp"
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
    void evaluate(float frame);
    void clear();
    bool isEmpty() const;

    template<typename T>
    T sampleAt(Field& field, float frame) const {
        const auto it = tracks.find(&field);
        if (it == tracks.end() || it->second.isEmpty()) return field.get<T>();
        return it->second.sample<T>(frame);
    }

private:
    std::unordered_map<Field*, Track> tracks;
};
