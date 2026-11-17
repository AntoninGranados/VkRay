#pragma once

#include <string>

#include "core/ecs/components/component.hpp"
#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

class ComponentInspector {
public:
    virtual ~ComponentInspector() = default;

    virtual bool replacesFields() const { return false; }
    virtual bool draw(Component& component, Registry& registry, Entity entity) = 0;
};

class MeshInspector : public ComponentInspector {
public:
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class TextureInspector : public ComponentInspector {
public:
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class MaterialInspector : public ComponentInspector {
public:
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class MaterialRefInspector : public ComponentInspector {
public:
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class PhysicsInspector : public ComponentInspector {
public:
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class MeshSimplifyInspector : public ComponentInspector {
public:
    bool replacesFields() const override { return true; }
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class CompositingInspector : public ComponentInspector {
public:
    bool replacesFields() const override { return true; }
    bool draw(Component& component, Registry& registry, Entity entity) override;
};

class ScriptInspector : public ComponentInspector {
public:
    explicit ScriptInspector(const ScriptKind& kind) : kind(kind) {}

    bool replacesFields() const override { return true; }
    bool draw(Component& component, Registry& registry, Entity entity) override;

    static bool drawPluginErrorAndFields(ShaderPlugin& plugin, const std::string& idSuffix);

private:
    const ScriptKind& kind;
};

} // namespace ecs
