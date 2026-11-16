#include "scene_serializer.hpp"
#include "scene_serializer_internal.hpp"

#include <filesystem>
#include <format>
#include <random>
#include <unordered_set>

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/compositing.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/material.hpp"
#include "core/ecs/systems/mesh_system.hpp"
#include "core/fields/field_serializer.hpp"
#include "core/render/script_table.hpp"
#include "gltf_loader.hpp"
#include "scene.hpp"
#include "utils/log.hpp"

using json = nlohmann::ordered_json;

namespace {

void reparseScripts(ecs::Registry& registry) {
    for (const ScriptKind* kind : ScriptTable::componentKinds()) {
        for (const ecs::ComponentType* type : kind->components) {
            for (const ecs::Entity entity : registry.storage(*type).entities()) {
                ecs::Component& script = registry.get(entity, *type);
                script.payload<ShaderPlugin>("plugin").parse(ScriptTable::scriptPath(script), *kind);
            }
        }
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
        pass.plugin->parse(pass.path, ScriptTable::kCompositing);

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
        json scriptParams = d.value;
        for (const Field& field : d.componentType->getFields()) scriptParams.erase(field.getId().generic_string());
        applyComponent(scriptParams, comp, animStore, d.ctx);
        warnUnknownFields(d.value, comp);
    }
}

void activateFirstCamera(ecs::Registry& registry, Scene& scene) {
    for (const ecs::Entity& entity : registry.storage(ecs::Camera).entities()) {
        scene.setActiveCamera(entity);
        break;
    }
}

} // namespace

bool SceneSerializer::load(Scene& scene, const std::string& path, std::optional<uint32_t> forceSeed) {
    if (GltfLoader::handles(path)) {
        if (!GltfLoader::load(scene, path)) return false;
        reparseScripts(scene.getRegistry());
        activateFirstCamera(scene.getRegistry(), scene);
        return true;
    }

    std::optional<json> parsed = parseSceneFile(path);
    if (!parsed) return false;
    const json& j = *parsed;

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

    resolveScenePaths(spawn.registry, std::filesystem::absolute(path).parent_path());
    resolveDeferredEntityFields(spawn.registry, spawn.deferredEntityFields, buildEntityNameMap(scene, spawn.registry));
    reloadMeshAssets(spawn.registry, scene);

    reparseScripts(spawn.registry);
    applyMeshSimplification(spawn.registry, scene);
    parseCompositingPlugins(spawn.registry, spawn.deferredCompositingParams);
    applyDeferredPluginParams(spawn.registry, spawn.animStore, spawn.deferredPluginParams);
    activateFirstCamera(spawn.registry, scene);

    spawn.animStore.evaluate(0.0f);
    return true;
}
