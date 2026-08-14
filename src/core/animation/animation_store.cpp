#include "animation_store.hpp"

void AnimationStore::capture(ecs::Entity e, ecs::Component& component, const std::string& fieldId, int frame) {
    Track& track = entityTracks[{e, &component.getType(), fieldId}];
    track.setKeyframe(frame, component.getField(fieldId));
}

void AnimationStore::insert(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame, FieldValue value, Interpolation interp) {
    Track& track = entityTracks[{e, &type, fieldId}];
    track.setKeyframe(frame, std::move(value), interp);
}

bool AnimationStore::has(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame) const {
    auto it = entityTracks.find({e, &type, fieldId});
    return it != entityTracks.end() && it->second.has(frame);
}

void AnimationStore::remove(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame) {
    auto it = entityTracks.find({e, &type, fieldId});
    if (it != entityTracks.end()) it->second.erase(frame);
}

void AnimationStore::setInterpolation(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame, Interpolation interp) {
    auto it = entityTracks.find({e, &type, fieldId});
    if (it != entityTracks.end()) it->second.setInterpolation(frame, interp);
}

const std::map<int, Keyframe>& AnimationStore::keyframes(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId) const {
    static const std::map<int, Keyframe> empty;
    auto it = entityTracks.find({e, &type, fieldId});
    return it != entityTracks.end() ? it->second.getKeyframes() : empty;
}

void AnimationStore::remove(ecs::Entity e) {
    std::erase_if(entityTracks, [&](const auto& kv) { return kv.first.entity == e; });
}

void AnimationStore::clear() {
    entityTracks.clear();
}

bool AnimationStore::isEmpty() const {
    for (const auto& [key, track] : entityTracks)
        if (!track.isEmpty()) return false;
    return true;
}

void AnimationStore::evaluate(ecs::Registry& registry, float frame) {
    for (auto& [key, track] : entityTracks) {
        if (track.isEmpty()) continue;
        if (!registry.has(key.entity, *key.type)) continue;
        ecs::Component& component = registry.get(key.entity, *key.type);
        Field& field = component.getField(key.fieldId);
        field.dispatch([&]<typename T>(T) { field.set<T>(track.sample<T>(frame)); });
    }
}
