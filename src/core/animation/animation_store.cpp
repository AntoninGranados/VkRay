#include "animation_store.hpp"

#include <utility>

#include "core/animation/keyframe.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/fields/field.hpp"

void AnimationStore::capture(Field& field, int frame, Interpolation interp) {
    tracks[&field].insert_or_assign(frame, Keyframe{field, interp});
}

void AnimationStore::insert(Field& field, FieldValue value, int frame, Interpolation interp) {
    tracks[&field].insert_or_assign(frame, Keyframe{std::move(value), interp});
}

bool AnimationStore::has(Field& field, int frame) const {
    auto it = tracks.find(&field);
    return it != tracks.end() && it->second.contains(frame);
}

void AnimationStore::remove(Field& field, int frame) {
    auto it = tracks.find(&field);
    if (it != tracks.end()) it->second.erase(frame);
}

void AnimationStore::setInterpolation(Field& field, int frame, Interpolation interp) {
    auto it = tracks.find(&field);
    if (it == tracks.end()) return;
    auto keyframe = it->second.find(frame);
    if (keyframe != it->second.end()) keyframe->second.interpolation = interp;
}

const std::map<int, Keyframe>& AnimationStore::keyframes(Field& field) const {
    static const std::map<int, Keyframe> empty;
    auto it = tracks.find(&field);
    return it != tracks.end() ? it->second : empty;
}

void AnimationStore::remove(ecs::Registry& registry, ecs::Entity& e) {
    for (const ecs::ComponentType* type : ecs::ComponentType::all()) {
        if (!registry.has(e, *type)) continue;

        for (Field& field : registry.get(e, *type).getFields()) { tracks.erase(&field); }
    }
}

void AnimationStore::clear() { tracks.clear(); }

bool AnimationStore::isEmpty() const {
    for (const auto& [field, track] : tracks)
        if (!track.empty()) return false;
    return true;
}

std::unordered_set<const Field*> AnimationStore::evaluate(float frame) {
    std::unordered_set<const Field*> evaluated;
    for (auto& [field, track] : tracks) {
        if (track.empty()) continue;

        field->dispatch([&]<typename T>(T) { field->set<T>(sample<T>(track, frame)); });
        evaluated.insert(field);
    }
    return evaluated;
}
