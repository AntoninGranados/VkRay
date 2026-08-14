#pragma once

#include <functional>
#include <map>
#include <string>
#include <unordered_map>

#include "core/animation/keyframe.hpp"
#include "core/animation/track.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

class AnimationStore {
public:
    void capture(ecs::Entity e, ecs::Component& component, const std::string& fieldId, int frame);
    void insert(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame, FieldValue value, Interpolation interp = Interpolation::Linear);
    bool has(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame) const;
    void remove(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame);
    void setInterpolation(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId, int frame, Interpolation interp);
    const std::map<int, Keyframe>& keyframes(ecs::Entity e, const ecs::ComponentType& type, const std::string& fieldId) const;
    void remove(ecs::Entity e);
    void evaluate(ecs::Registry& registry, float frame);
    void clear();
    bool isEmpty() const;

private:
    struct EntityFieldKey {
        ecs::Entity entity;
        const ecs::ComponentType* type;
        std::string fieldId;
        bool operator==(const EntityFieldKey&) const = default;
    };
    struct EntityFieldKeyHash {
        size_t operator()(const EntityFieldKey& k) const {
            size_t h = std::hash<ecs::Entity>{}(k.entity);
            h ^= std::hash<const ecs::ComponentType*>{}(k.type) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<std::string>{}(k.fieldId) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<EntityFieldKey, Track, EntityFieldKeyHash> entityTracks;
};
