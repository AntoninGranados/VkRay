#include "script_table.hpp"

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

const ScriptKind ScriptTable::kMaterial = {
    "IMaterial", {"IBsdf", "IMaterial"}, {"DefaultMaterial"}, &ecs::MaterialPlugin};
const ScriptKind ScriptTable::kSky = {"ISky", {"ISky"}, {"DefaultSky"}, &ecs::SkyPlugin};
const ScriptKind ScriptTable::kLens = {"ILens", {"ILens"}, {"DefaultLens"}, &ecs::CameraLensPlugin};
const ScriptKind ScriptTable::kCompositing = {"ICompositingPass", {"ICompositingPass"}, {"IdentityPass"}};

namespace {

int packPlugin(ShaderPlugin& plugin, const std::filesystem::path& path, const ScriptKind& kind, int fallback,
               std::vector<float>& params) {
    if (!plugin.parse(path, kind)) return fallback;

    const std::vector<float> values = plugin.packValues();
    params.insert(params.end(), values.begin(), values.end());
    return plugin.getSlot();
}

} // namespace

const std::array<const ScriptKind*, 3>& ScriptTable::componentKinds() {
    static const std::array<const ScriptKind*, 3> kinds = {&kMaterial, &kSky, &kLens};
    return kinds;
}

int ScriptTable::packScript(ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind, int fallback,
                            std::vector<float>& params) {
    if (entity == ecs::Entity{} || !registry.has(entity, *kind.component)) return fallback;

    ecs::Component& c = registry.get(entity, *kind.component);
    return packPlugin(c.payload<ShaderPlugin>("plugin"), c.get<std::filesystem::path>("path"), kind, fallback, params);
}

int ScriptTable::packCompositingPass(ecs::CompositingPassEntry& pass, std::vector<float>& params) {
    return packPlugin(*pass.plugin, pass.path, kCompositing, 0, params);
}

const ShaderPlugin* ScriptTable::materialPlugin(const ecs::Registry& registry, ecs::Entity material) {
    if (!registry.has(material, ecs::MaterialPlugin)) return nullptr;
    return &registry.get(material, ecs::MaterialPlugin).payload<ShaderPlugin>("plugin");
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
