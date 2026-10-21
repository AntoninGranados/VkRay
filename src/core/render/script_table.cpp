#include "script_table.hpp"

#include <bit>
#include <filesystem>

#include <glm/glm.hpp>

#include "VkSmol/render/shader.hpp"

#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/environment.hpp"
#include "core/ecs/components/material.hpp"
#include "core/ecs/registry.hpp"
#include "core/scene/gpu_structs.hpp"
#include "core/scene/scene.hpp"

const ScriptKind ScriptTable::kMaterial = {
    "IMaterial",
    "ScriptMaterial",
    {"PrincipledMaterial", "EmissiveMaterial", "DiffuseMaterial", "MetalMaterial", "GlossyMaterial",
     "DielectricMaterial", "VolumeMaterial"},
};
const ScriptKind ScriptTable::kSky = {"ISky", "ScriptSky", {"DefaultSky"}};
const ScriptKind ScriptTable::kLens = {"ILens", "ScriptLens", {}};
const ScriptKind ScriptTable::kCompositing = {"ICompositingPass", "ScriptPass", {"IdentityPass"}};

namespace {

const ecs::ComponentType* const kMaterialComponents[] = {
    &ecs::Principled, &ecs::Emissive, &ecs::Diffuse, &ecs::Metal, &ecs::Glossy, &ecs::Dielectric, &ecs::Volume,
};

constexpr const char* kTiltShiftPath = "builtin:/camera/tilt_shift.slang";

ShaderPlugin& tiltShiftPlugin() {
    static ShaderPlugin plugin;
    plugin.parse(kTiltShiftPath, ScriptTable::kLens);
    return plugin;
}

void packFields(const ecs::Component& c, std::vector<float>& params) {
    for (const Field& field : c.getFields()) {
        if (field.getType() == FieldType::Vec3) {
            const glm::vec3 value = field.get<glm::vec3>();
            params.insert(params.end(), {value.x, value.y, value.z});
        } else if (field.getType() == FieldType::Int) {
            params.push_back(std::bit_cast<float>(field.get<int>()));
        } else if (field.getType() == FieldType::Float) {
            params.push_back(field.get<float>());
        }
    }
}

int packScript(ShaderPlugin& plugin, const std::filesystem::path& path, const ScriptKind& kind, int fallback,
               std::vector<float>& params) {
    if (!plugin.parse(path, kind)) return fallback;

    const std::vector<float> values = plugin.packValues();
    params.insert(params.end(), values.begin(), values.end());
    return plugin.getSlot();
}

} // namespace

bool ScriptTable::packMaterial(ecs::Registry& registry, ecs::Entity entity, GpuMaterial& gpu,
                               std::vector<float>& params) {
    gpu.base = static_cast<uint32_t>(params.size());

    for (int i = 0; i < static_cast<int>(std::size(kMaterialComponents)); i++) {
        if (!registry.has(entity, *kMaterialComponents[i])) continue;

        gpu.type = i;
        packFields(registry.get(entity, *kMaterialComponents[i]), params);
        return true;
    }

    if (!registry.has(entity, ecs::MaterialPlugin)) return false;

    ecs::Component& c = registry.get(entity, ecs::MaterialPlugin);
    gpu.type =
        packScript(c.payload<ShaderPlugin>("plugin"), c.get<std::filesystem::path>("path"), kMaterial, -1, params);
    return gpu.type >= 0;
}

int ScriptTable::packSky(ecs::Registry& registry, ecs::Entity environment, std::vector<float>& params) {
    if (environment == ecs::Entity{} || !registry.has(environment, ecs::SkyPlugin)) return 0;

    ecs::Component& c = registry.get(environment, ecs::SkyPlugin);
    return packScript(c.payload<ShaderPlugin>("plugin"), c.get<std::filesystem::path>("path"), kSky, 0, params);
}

int ScriptTable::packLens(ecs::Registry& registry, ecs::Entity camera, std::vector<float>& params) {
    if (registry.has(camera, ecs::CameraLensPlugin)) {
        ecs::Component& c = registry.get(camera, ecs::CameraLensPlugin);
        return packScript(c.payload<ShaderPlugin>("plugin"), c.get<std::filesystem::path>("path"), kLens, -1, params);
    }

    if (registry.has(camera, ecs::TiltShiftLens)) {
        ShaderPlugin& plugin = tiltShiftPlugin();
        if (!plugin.getError().empty()) return -1;

        packFields(registry.get(camera, ecs::TiltShiftLens), params);
        return plugin.getSlot();
    }

    return -1;
}

int ScriptTable::packCompositingPass(ecs::CompositingPassEntry& pass, std::vector<float>& params) {
    return packScript(*pass.plugin, pass.path, kCompositing, 0, params);
}

std::vector<ecs::CompositingPassEntry>& ScriptTable::compositingPasses(ecs::Registry& registry) {
    const ecs::Entity compositing = Scene::findSceneRootChild(registry, ecs::Compositing);
    if (compositing == ecs::Entity{}) {
        static std::vector<ecs::CompositingPassEntry> empty;
        return empty;
    }
    return registry.get(compositing, ecs::Compositing).payload<ecs::CompositingPasses>("passes").passes;
}

int ScriptTable::compositingDispatchCount(ecs::Registry& registry) {
    int total = 0;
    for (ecs::CompositingPassEntry& pass : compositingPasses(registry)) {
        pass.plugin->parse(pass.path, kCompositing);
        total += pass.plugin->getPassCount();
    }
    return total;
}

std::vector<ShaderConformance> ScriptTable::conformances(std::initializer_list<const ScriptKind*> kinds) {
    tiltShiftPlugin();

    std::vector<ShaderConformance> result;
    for (const ScriptKind* kind : kinds) ShaderPlugin::appendConformances(*kind, result);
    return result;
}
