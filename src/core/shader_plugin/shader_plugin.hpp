#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"

struct ShaderConformance;
struct ShaderField;

struct ScriptKind {
    std::string interfaceName;
    std::string wrapperName;
    std::vector<std::string> builtins;
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

    static void appendConformances(const ScriptKind& kind, std::vector<ShaderConformance>& conformances);

    const std::string& getError() const { return error; }
    ecs::Component& getComponent() { return params; }
    const ecs::Component& getComponent() const { return params; }

    std::vector<float> packValues() const;

    const std::string& getSource() const { return source; }
    int getSlot() const { return slot; }
    int getPassCount() const { return passCount; }

private:
    static std::vector<ShaderPlugin*>& registry();

    void load(bool migrate);
    bool reflect(std::vector<Field>& fields);
    bool appendFields(const std::vector<ShaderField>& shaderFields, const std::string& group,
                      std::vector<Field>& fields);

    ecs::ComponentType schema;
    ecs::Component params{schema};

    std::optional<std::filesystem::path> path;
    const ScriptKind* kind = nullptr;
    int slot = -1;
    int passCount = 1;
    std::optional<size_t> watchId;

    std::string error;
    std::string source;
    std::string structName;
};
