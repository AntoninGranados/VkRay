#include "animation_store.hpp"
#include "core/animation/keyframe.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/fields/field.hpp"

void AnimationStore::capture(Field& field, int frame, Interpolation interp) {
    tracks[&field].setKeyframe(frame, field, interp);
}

void AnimationStore::insert(Field& field, FieldValue value, int frame, Interpolation interp) {
    tracks[&field].setKeyframe(frame, value, interp);
}

bool AnimationStore::has(Field& field, int frame) const {
    auto it = tracks.find(&field);
    return it != tracks.end() && it->second.has(frame);
}

void AnimationStore::remove(Field& field, int frame) {
    auto it = tracks.find(&field);
    if (it != tracks.end()) it->second.erase(frame);
}

void AnimationStore::setInterpolation(Field& field, int frame, Interpolation interp) {
    auto it = tracks.find(&field);
    if (it != tracks.end()) it->second.setInterpolation(frame, interp);
}

const std::map<int, Keyframe>& AnimationStore::keyframes(Field& field) const {
    static const std::map<int, Keyframe> empty;
    auto it = tracks.find(&field);
    return it != tracks.end() ? it->second.getKeyframes() : empty;
}

void AnimationStore::remove(ecs::Registry& registry, ecs::Entity& e) {
    for (const ecs::ComponentType& type : ecs::ComponentType::all()) {
        if (!registry.has(e, type)) continue;

        for (Field& field : registry.get(e, type).getFields()) {
            tracks.erase(&field);
        }
    }
}

void AnimationStore::clear() {
    tracks.clear();
}

bool AnimationStore::isEmpty() const {
    for (const auto& [field, track] : tracks)
        if (!track.isEmpty()) return false;
    return true;
}

void AnimationStore::evaluate(float frame) {
    for (auto& [field, track] : tracks) {
        if (track.isEmpty()) continue;

        field->dispatch([&]<typename T>(T) { field->set<T>(track.sample<T>(frame)); });
    }
}
