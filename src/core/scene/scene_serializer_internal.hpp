#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"

#include "core/animation/animation_store.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "utils/json_dsl.hpp"

class Scene;

inline constexpr int kSceneVersion = 1;

struct DeferredEntityField {
    ecs::Entity entity;
    const ecs::ComponentType* componentType;
    std::string fieldId;
    std::string entityName;
};

struct DeferredPluginParams {
    ecs::Entity entity;
    const ecs::ComponentType* componentType;
    nlohmann::ordered_json value;
    ResolveCtx ctx;
};

struct DeferredCompositingParams {
    ecs::Entity entity;
    size_t passIndex;
    nlohmann::ordered_json params;
    ResolveCtx ctx;
};

struct SpawnContext {
    ecs::Registry& registry;
    AnimationStore& animStore;
    std::vector<DeferredEntityField> deferredEntityFields;
    std::vector<DeferredPluginParams> deferredPluginParams;
    std::vector<DeferredCompositingParams> deferredCompositingParams;
};

std::optional<nlohmann::ordered_json> parseSceneFile(const std::string& path);
uint32_t resolveSeed(const nlohmann::ordered_json& j, std::optional<uint32_t> forceSeed);
void applyComponent(const nlohmann::ordered_json& obj, ecs::Component& comp, AnimationStore& animStore,
                    const ResolveCtx& ctx);
void warnUnknownFields(const nlohmann::ordered_json& obj, ecs::Component& comp);

void loadSection(const nlohmann::ordered_json& j, const std::string& key, ecs::Entity root,
                 const ResolveCtx& resolveCtx, SpawnContext& spawn);
std::unordered_map<std::string, ecs::Entity> buildEntityNameMap(const Scene& scene, const ecs::Registry& registry);
void resolveDeferredEntityFields(ecs::Registry& registry, const std::vector<DeferredEntityField>& deferredEntityFields,
                                 const std::unordered_map<std::string, ecs::Entity>& entityByName);
void reloadMeshAssets(ecs::Registry& registry, const Scene& scene);
void replaceSceneRootIfProvided(const nlohmann::ordered_json& j, Scene& scene);
void ensureSceneDefaults(Scene& scene);
