#include "scene_serializer.hpp"
#include "scene_serializer_internal.hpp"

#include <filesystem>
#include <format>
#include <random>
#include <unordered_set>

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/compositing.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/material.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "core/fields/field_serializer.hpp"
#include "core/render/camera_lens_table.hpp"
#include "core/render/compositing_table.hpp"
#include "core/render/material_table.hpp"
#include "core/render/sky_table.hpp"
#include "scene.hpp"
#include "utils/log.hpp"

using json = nlohmann::ordered_json;

namespace {

void reparsePlugins(ecs::Registry& registry, const ecs::ComponentType& type, const std::string& tableType,
                    int tableVersion, int (*slotFor)(const std::filesystem::path&)) {
    for (const ecs::Entity entity : registry.storage(type).entities()) {
        ecs::Component& pluginComp = registry.get(entity, type);
        const std::filesystem::path path = pluginComp.get<std::filesystem::path>("path");
        pluginComp.payload<ShaderPlugin>("plugin").parse(path, tableType, tableVersion, slotFor(path));
    }
}

void applyMeshSimplification(ecs::Registry& registry, const Scene& scene) {
    for (const ecs::Entity entity : scene.getChildren(scene.getAssetsRoot())) {
        if (!registry.has(entity, ecs::MeshSimplify)) continue;
        const float ratio = registry.get(entity, ecs::MeshSimplify).get<float>("ratio");
        ecs::requestMeshSimplify(registry, entity, ratio);
    }
}

void parseCompositingPlugins(ecs::Registry& registry, const std::vector<DeferredCompositingParams>& deferred) {
    for (const DeferredCompositingParams& d : deferred) {
        ecs::Component& comp = registry.get(d.entity, ecs::Compositing);
        ecs::CompositingPassEntry& pass = comp.payload<ecs::CompositingPasses>("passes").passes[d.passIndex];
        pass.plugin->parse(pass.path, CompositingTable::kType, CompositingTable::kVersion,
                           CompositingTable::slotFor(pass.path));

        std::unordered_set<std::string> knownIds;
        for (Field& f : pass.plugin->getComponent().getFields()) {
            const std::string id = f.getId().generic_string();
            knownIds.insert(id);
            if (d.params.contains(id)) applyField(d.params[id], f, d.ctx);
        }
        for (const auto& [key, value] : d.params.items())
            if (!knownIds.contains(key))
                Log::warn("SceneSerializer",
                          std::format("Unknown compositing pass param '{}' on '{}'", key, pass.name));
    }
}

void applyDeferredPluginParams(ecs::Registry& registry, AnimationStore& animStore,
                               const std::vector<DeferredPluginParams>& deferred) {
    for (const DeferredPluginParams& d : deferred) {
        ecs::Component& comp = registry.get(d.entity, *d.componentType);
        applyComponent(d.value, comp, animStore, d.ctx);
        warnUnknownFields(d.value, comp);
    }
}

void activateFirstNonDefaultCamera(ecs::Registry& registry, Scene& scene) {
    for (const ecs::Entity& entity : registry.storage(ecs::Camera).entities()) {
        if (entity == scene.getDefaultCamera()) continue;
        scene.setActiveCamera(entity);
        break;
    }
}

} // namespace

bool SceneSerializer::load(Scene& scene, const std::string& path, std::optional<uint32_t> forceSeed) {
    std::optional<json> parsed = parseSceneFile(path);
    if (!parsed) return false;
    const json& j = *parsed;

    Core::getEngine().waitIdle();
    scene.clear();

    const int version = j.value("version", -1);
    if (version != kSceneVersion) {
        Log::error("SceneSerializer",
                   std::format("Scene version mismatch in '{}': expected {}, got {}", path, kSceneVersion, version));
        return false;
    }

    std::mt19937 rng(resolveSeed(j, forceSeed));
    ResolveCtx ctx{rng, {}};

    replaceSceneRootIfProvided(j, scene);

    SpawnContext spawn{scene.getRegistry(), scene.getAnimationStore(), {}, {}, {}};

    loadSection(j, "Scene", scene.getSceneRoot(), ctx, spawn);
    ensureSceneDefaults(scene);
    loadSection(j, "Materials", scene.getMaterialsRoot(), ctx, spawn);
    loadSection(j, "Assets", scene.getAssetsRoot(), ctx, spawn);
    loadSection(j, "Objects", scene.getObjectsRoot(), ctx, spawn);

    resolveDeferredEntityFields(spawn.registry, spawn.deferredEntityFields, buildEntityNameMap(scene, spawn.registry));
    reloadMeshAssets(spawn.registry, scene);

    reparsePlugins(spawn.registry, ecs::MaterialPlugin, MaterialTable::kType, MaterialTable::kVersion,
                   MaterialTable::slotFor);
    reparsePlugins(spawn.registry, ecs::CameraLensPlugin, CameraLensTable::kType, CameraLensTable::kVersion,
                   CameraLensTable::slotFor);
    reparsePlugins(spawn.registry, ecs::SkyPlugin, SkyTable::kType, SkyTable::kVersion, SkyTable::slotFor);
    applyMeshSimplification(spawn.registry, scene);
    parseCompositingPlugins(spawn.registry, spawn.deferredCompositingParams);
    applyDeferredPluginParams(spawn.registry, spawn.animStore, spawn.deferredPluginParams);
    activateFirstNonDefaultCamera(spawn.registry, scene);

    spawn.animStore.evaluate(0.0f);
    return true;
}
