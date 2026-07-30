#include "animation_store.hpp"

#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

Track& AnimationStore::getTrack(ecs::Entity e, const ecs::ComponentType& type, const ecs::Field& field) {
    return entityTracks[e][type.getId()][field.id];
}

Track& AnimationStore::getTrack(MaterialHandle handle, const std::string& fieldId) {
    return materialTracks[handle][fieldId];
}

void AnimationStore::remove(ecs::Entity e) {
    entityTracks.erase(e);
}

void AnimationStore::remove(MaterialHandle handle) {
    materialTracks.erase(handle);
}

void AnimationStore::clear() {
    entityTracks.clear();
    materialTracks.clear();
}

bool AnimationStore::isEmpty() const {
    for (const auto& [e, componentTracks] : entityTracks)
        for (const auto& [typeId, fieldTracks] : componentTracks)
            for (const auto& [fieldId, track] : fieldTracks)
                if (!track.isEmpty()) return false;
    for (const auto& [handle, fieldTracks] : materialTracks)
        for (const auto& [fieldId, track] : fieldTracks)
            if (!track.isEmpty()) return false;
    return true;
}

void AnimationStore::evaluate(ecs::Registry& registry, float frame) {
    for (auto& [entity, componentTracks] : entityTracks) {
        for (auto& [typeId, fieldTracks] : componentTracks) {
            const ecs::ComponentType* compType = nullptr;
            for (const auto& ct : ecs::ComponentType::all())
                if (ct.getId() == typeId) { compType = &ct; break; }
            if (!compType || !registry.has(entity, *compType)) continue;

            ecs::Component& component = registry.get(entity, *compType);
            for (auto& [fieldId, track] : fieldTracks) {
                if (track.isEmpty()) continue;
                writeValue(component, compType->getField(fieldId), interpolatedValue(track, frame));
            }
        }
    }
}

void AnimationStore::evaluate(std::vector<Material>& materials, float frame) {
    for (auto& [handle, fieldTracks] : materialTracks) {
        if (handle < 0 || handle >= static_cast<int>(materials.size())) continue;
        Material& mat = materials[handle];
        for (auto& [fieldId, track] : fieldTracks) {
            if (track.isEmpty()) continue;
            writeValue(mat, fieldId, interpolatedValue(track, frame));
        }
    }
}

KeyframeValue AnimationStore::sampleValue(const ecs::Component& component, const ecs::Field& field) {
    switch (field.type) {
        case ecs::FieldType::Bool:   return component.get<bool>(field.id);
        case ecs::FieldType::Int:    return component.get<int>(field.id);
        case ecs::FieldType::Int2:   return component.get<glm::ivec2>(field.id);
        case ecs::FieldType::Int3:   return component.get<glm::ivec3>(field.id);
        case ecs::FieldType::Int4:   return component.get<glm::ivec4>(field.id);
        case ecs::FieldType::Float:  return component.get<float>(field.id);
        case ecs::FieldType::Float2: return component.get<glm::vec2>(field.id);
        case ecs::FieldType::Float3: return component.get<glm::vec3>(field.id);
        case ecs::FieldType::Float4: return component.get<glm::vec4>(field.id);
        case ecs::FieldType::Quat:   return component.get<glm::quat>(field.id);
        case ecs::FieldType::String: return component.get<std::string>(field.id);
    }
    std::unreachable();
}

KeyframeValue AnimationStore::sampleValue(const Material& mat, const std::string& field) {
    if (field == "albedo") return mat.albedo;
    if (field == "roughness") return mat.roughness;
    if (field == "metalness") return mat.metalness;
    if (field == "ior") return mat.ior;
    if (field == "transmission") return mat.transmission;
    if (field == "emissionStrength") return mat.emissionStrength;
    if (field == "density") return mat.density;
    if (field == "anisotropic") return mat.anisotropic;
    return 0.0f;
}

KeyframeValue AnimationStore::interpolatedValue(const Track& track, float frame) {
    const std::map<int, Keyframe>& keyframes = track.getKeyframes();
    if (frame <= float(keyframes.begin()->first)) return keyframes.begin()->second.value;
    if (frame >= float(keyframes.rbegin()->first)) return keyframes.rbegin()->second.value;
    const auto next = keyframes.upper_bound(static_cast<int>(frame));
    const auto prev = std::prev(next);
    const float t = (frame - float(prev->first)) / float(next->first - prev->first);
    return Keyframe::interpolate(prev->second, next->second, t);
}

void AnimationStore::writeValue(Material& mat, const std::string& fieldId, const KeyframeValue& value) {
    if (fieldId == "albedo") { mat.albedo = std::get<glm::vec3>(value); return; }
    if (fieldId == "roughness") { mat.roughness = std::get<float>(value); return; }
    if (fieldId == "metalness") { mat.metalness = std::get<float>(value); return; }
    if (fieldId == "ior") { mat.ior = std::get<float>(value); return; }
    if (fieldId == "transmission") { mat.transmission = std::get<float>(value); return; }
    if (fieldId == "emissionStrength") { mat.emissionStrength = std::get<float>(value); return; }
    if (fieldId == "density") { mat.density = std::get<float>(value); return; }
    if (fieldId == "anisotropic") { mat.anisotropic = std::get<float>(value); return; }
}

void AnimationStore::writeValue(ecs::Component& component, const ecs::Field& field, const KeyframeValue& value) {
    std::visit([&](const auto& v) {
        component.set<std::decay_t<decltype(v)>>(field.id, v);
    }, value);
}
