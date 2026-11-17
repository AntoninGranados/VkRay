#pragma once

#include <array>
#include <filesystem>
#include <initializer_list>
#include <vector>

#include "core/ecs/components/compositing.hpp"
#include "core/ecs/entity.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {
class Registry;
}

class Scene;

namespace VkSmol {
struct ShaderConformance;
}

namespace ScriptTable {
extern const ScriptKind kMaterial;
extern const ScriptKind kSky;
extern const ScriptKind kLens;
extern const ScriptKind kCompositing;

const std::array<const ScriptKind*, 3>& componentKinds();

ecs::Component* findScript(ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind);
const ecs::Component* findScript(const ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind);
std::filesystem::path scriptPath(const ecs::Component& script);

int packScript(ecs::Registry& registry, ecs::Entity entity, const ScriptKind& kind, int fallback,
               const TextureSlots& textureSlots, std::vector<float>& params);
int packCompositingPass(ecs::CompositingPassEntry& pass, const TextureSlots& textureSlots, std::vector<float>& params);

const ShaderPlugin* materialPlugin(const ecs::Registry& registry, ecs::Entity material);
const ecs::Component* tiltShift(const ecs::Registry& registry, ecs::Entity camera);
ecs::Component* tiltShift(ecs::Registry& registry, ecs::Entity camera);

std::vector<ecs::CompositingPassEntry>& compositingPasses(Scene& scene);
int compositingDispatchCount(Scene& scene);

std::vector<VkSmol::ShaderConformance> conformances(std::initializer_list<const ScriptKind*> kinds);
} // namespace ScriptTable
