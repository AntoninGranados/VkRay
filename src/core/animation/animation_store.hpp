#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "core/animation/keyframe.hpp"
#include "core/animation/track.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/scene/object/material.hpp"

class AnimationStore {
public:
    Track& getTrack(ecs::Entity e, const ecs::ComponentType& type, const ecs::Field& field);
    Track& getTrack(MaterialHandle handle, const std::string& fieldId);

    void remove(ecs::Entity e);
    void remove(MaterialHandle handle);
    void evaluate(ecs::Registry& registry, float frame);
    void evaluate(std::vector<Material>& materials, float frame);
    void clear();
    bool isEmpty() const;

    static KeyframeValue sampleValue(const ecs::Component& component, const ecs::Field& field);
    static KeyframeValue sampleValue(const Material& mat, const std::string& field);

private:
    static KeyframeValue interpolatedValue(const Track& track, float frame);
    static void writeValue(ecs::Component& component, const ecs::Field& field, const KeyframeValue& value);
    static void writeValue(Material& mat, const std::string& fieldId, const KeyframeValue& value);

    std::unordered_map<ecs::Entity, std::unordered_map<std::string, std::unordered_map<std::string, Track>>> entityTracks;
    std::unordered_map<MaterialHandle, std::unordered_map<std::string, Track>> materialTracks;
};
