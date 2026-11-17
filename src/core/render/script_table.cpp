#include "script_table.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <utility>

#include "VkSmol/render/shader.hpp"

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/material.hpp"
#include "core/ecs/registry.hpp"
#include "core/scene/scene.hpp"

const ScriptKind ScriptTable::kMaterial = {"IMaterial",
                                           {"IBsdf", "IMaterial"},
                                           {"DefaultMaterial"},
                                           {&ecs::MaterialPlugin, &ecs::Diffuse, &ecs::Emissive, &ecs::Metal,
                                            &ecs::Glossy, &ecs::Dielectric, &ecs::Volume, &ecs::Principled}};
const ScriptKind ScriptTable::kSky = {"ISky", {"ISky"}, {"DefaultSky"}, {&ecs::SkyPlugin}};
const ScriptKind ScriptTable::kLens = {"ILens", {"ILens"}, {"DefaultLens"}, {&ecs::CameraLensPlugin}};
const ScriptKind ScriptTable::kCompositing = {"ICompositingPass", {"ICompositingPass"}, {"IdentityPass"}, {}};

namespace {

int packPlugin(ShaderPlugin& plugin, const std::filesystem::path& path, const ScriptKind& kind, int fallback,
               const TextureSlots& textureSlots, std::vector<float>& params) {
    if (!plugin.parse(path, kind)) return fallback;

    const std::vector<float> values = plugin.packValues(textureSlots);
    params.insert(params.end(), values.begin(), values.end());
    return plugin.getSlot();
}

} // namespace

const std::array<const ScriptKind*, 3>& ScriptTable::componentKinds() {
    static const std::array<const ScriptKind*, 3> kinds = {&kMaterial, &kSky, &kLens};
    return kinds;
}

const ScriptKind* ScriptTable::kindOf(const ecs::ComponentType& type) {
    for (const ScriptKind* kind : componentKinds())
        if (std::ranges::any_of(kind->components,
                                [&](const ecs::ComponentType* component) { return *component == type; }))
            return kind;
    return nullptr;
}

const ecs::Component* ScriptTable::findScript(const ecs::Registry& registry, ecs::Entity entity,
                                              const ScriptKind& kind) {
    if (entity == ecs::Entity{}) return nullptr;
    for (const ecs::ComponentType* type : kind.components)
        if (registry.has(entity, *type)) return &registry.get(entity, *type);
    return nullptr;
}

ecs::Component* ScriptTable::findScript(ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind) {
    return const_cast<ecs::Component*>(findScript(std::as_const(registry), entity, kind));
}

std::filesystem::path ScriptTable::scriptPath(const ecs::Component& script) {
    const std::string& builtin = script.getType().getScript();
    return builtin.empty() ? script.get<std::filesystem::path>("path") : std::filesystem::path(builtin);
}

int ScriptTable::packScript(ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind, int fallback,
                            const TextureSlots& textureSlots, std::vector<float>& params) {
    ecs::Component* script = findScript(registry, entity, kind);
    if (!script) return fallback;
    return packPlugin(script->payload<ShaderPlugin>("plugin"), scriptPath(*script), kind, fallback, textureSlots,
                      params);
}

int ScriptTable::packCompositingPass(ecs::CompositingPassEntry& pass, const TextureSlots& textureSlots,
                                     std::vector<float>& params) {
    return packPlugin(*pass.plugin, pass.path, kCompositing, 0, textureSlots, params);
}

const ShaderPlugin* ScriptTable::materialPlugin(const ecs::Registry& registry, ecs::Entity material) {
    const ecs::Component* script = findScript(registry, material, kMaterial);
    return script ? &script->payload<ShaderPlugin>("plugin") : nullptr;
}

const ecs::Component* ScriptTable::tiltShift(const ecs::Registry& registry, ecs::Entity camera) {
    if (!registry.has(camera, ecs::CameraLensPlugin)) return nullptr;

    const ecs::Component& lens = registry.get(camera, ecs::CameraLensPlugin);
    if (lens.get<std::filesystem::path>("path") != ecs::kTiltShiftPath) return nullptr;
    return &lens.payload<ShaderPlugin>("plugin").getComponent();
}

ecs::Component* ScriptTable::tiltShift(ecs::Registry& registry, ecs::Entity camera) {
    return const_cast<ecs::Component*>(tiltShift(std::as_const(registry), camera));
}

std::vector<ecs::CompositingPassEntry>& ScriptTable::compositingPasses(Scene& scene) {
    const ecs::Entity compositing = scene.getCompositing();
    if (compositing == ecs::Entity{}) {
        static std::vector<ecs::CompositingPassEntry> empty;
        return empty;
    }
    return scene.getRegistry().get(compositing, ecs::Compositing).payload<ecs::CompositingPasses>("passes").passes;
}

int ScriptTable::compositingDispatchCount(Scene& scene) {
    int total = 0;
    for (ecs::CompositingPassEntry& pass : compositingPasses(scene)) {
        pass.plugin->parse(pass.path, kCompositing);
        total += pass.plugin->getPassCount();
    }
    return total;
}

std::vector<VkSmol::ShaderConformance> ScriptTable::conformances(std::initializer_list<const ScriptKind*> kinds) {
    std::vector<VkSmol::ShaderConformance> result;
    for (const ScriptKind* kind : kinds) ShaderPlugin::appendConformances(*kind, result);
    return result;
}
