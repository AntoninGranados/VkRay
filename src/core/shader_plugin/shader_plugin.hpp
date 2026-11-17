#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"

namespace VkSmol {
struct ShaderConformance;
struct ShaderField;
} // namespace VkSmol

using TextureSlots = std::unordered_map<ecs::Entity, int>;

struct ScriptKind {
    std::string dispatchInterface;
    std::vector<std::string> interfaces;
    std::vector<std::string> defaults;
    std::vector<const ecs::ComponentType*> components;
};

class ShaderPlugin {
public:
    ShaderPlugin();
    ~ShaderPlugin();
    ShaderPlugin(const ShaderPlugin&) = delete;
    ShaderPlugin& operator=(const ShaderPlugin&) = delete;
    ShaderPlugin(ShaderPlugin&&) = delete;
    ShaderPlugin& operator=(ShaderPlugin&&) = delete;

    bool parse(const std::filesystem::path& path, const ScriptKind& kind);

    static void appendConformances(const ScriptKind& kind, std::vector<VkSmol::ShaderConformance>& conformances);

    const std::string& getError() const { return error; }
    ecs::Component& getComponent() { return params; }
    const ecs::Component& getComponent() const { return params; }

    std::vector<float> packValues(const TextureSlots& textureSlots) const;

    const std::string& getSource() const { return source; }
    int getSlot() const { return slot; }
    int getPassCount() const { return passCount; }
    void setParameter(const std::string& id, std::vector<float> values) { pending[id] = std::move(values); }
    void setParameter(const std::string& id, ecs::Entity entity) { pendingEntities[id] = entity; }
    bool hasAttribute(const std::string& name) const { return std::ranges::contains(attributes, name); }

private:
    static std::vector<ShaderPlugin*>& registry();

    void load(bool migrate);
    bool reflect(std::vector<Field>& fields);
    bool appendFields(const std::vector<VkSmol::ShaderField>& shaderFields, const std::string& group,
                      std::vector<Field>& fields);

    ecs::ComponentType schema;
    ecs::Component params{schema};

    std::optional<std::filesystem::path> path;
    const ScriptKind* kind = nullptr;
    std::vector<std::string> attributes;
    std::unordered_map<std::string, std::vector<float>> pending;
    std::unordered_map<std::string, ecs::Entity> pendingEntities;
    int slot = -1;
    int passCount = 1;
    std::optional<size_t> watchId;

    std::string error;
    std::string source;
    std::string structName;
};
