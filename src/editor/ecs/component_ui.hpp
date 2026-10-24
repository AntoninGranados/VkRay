#pragma once

#include <string>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

class ComponentUi {
public:
    static void draw(Registry& registry, Entity entity);

private:
    static bool drawComponent(Component& component, Registry& registry, Entity entity);
    static bool drawMeshSimplify(Component& component, Registry& registry, Entity entity);
    static bool drawCompositing(Component& component);
    static bool drawShaderPluginField(Component& component, const ScriptKind& kind);
    static bool drawPluginErrorAndFields(ShaderPlugin& plugin, const std::string& idSuffix);
};

} // namespace ecs
