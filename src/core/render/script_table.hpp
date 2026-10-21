#pragma once

#include <initializer_list>
#include <vector>

#include "core/ecs/components/compositing.hpp"
#include "core/ecs/entity.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {
class Registry;
}

struct GpuMaterial;
struct ShaderConformance;

namespace ScriptTable {
extern const ScriptKind kMaterial;
extern const ScriptKind kSky;
extern const ScriptKind kLens;
extern const ScriptKind kCompositing;

bool packMaterial(ecs::Registry& registry, ecs::Entity entity, GpuMaterial& gpu, std::vector<float>& params);
int packSky(ecs::Registry& registry, ecs::Entity environment, std::vector<float>& params);
int packLens(ecs::Registry& registry, ecs::Entity camera, std::vector<float>& params);
int packCompositingPass(ecs::CompositingPassEntry& pass, std::vector<float>& params);

std::vector<ecs::CompositingPassEntry>& compositingPasses(ecs::Registry& registry);
int compositingDispatchCount(ecs::Registry& registry);

std::vector<ShaderConformance> conformances(std::initializer_list<const ScriptKind*> kinds);
} // namespace ScriptTable
