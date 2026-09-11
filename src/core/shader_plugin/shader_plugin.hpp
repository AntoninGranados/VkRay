#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"

class ShaderPlugin {
public:
    ShaderPlugin();
    ~ShaderPlugin();
    ShaderPlugin(const ShaderPlugin&) = delete;
    ShaderPlugin& operator=(const ShaderPlugin&) = delete;
    ShaderPlugin(ShaderPlugin&&) = delete;
    ShaderPlugin& operator=(ShaderPlugin&&) = delete;

    bool parse(const std::filesystem::path& path, const std::string& type, int version, int slot);

    static std::vector<ShaderPlugin*>& registry();

    const std::string& getError() const { return error; }
    ecs::Component& getComponent() { return params; }
    const ecs::Component& getComponent() const { return params; }

    struct PluginParam {
        const Field* field;
        std::string mangled;
        const char* glslType;
        int components;
        bool isInt;
        bool isBool;
    };
    std::vector<PluginParam> getParameters() const;

    std::vector<float> packValues() const;

    const std::string& getBody() const { return statements; }
    const std::string& getDeclarations() const { return declarations; }

    const std::string& getType() const { return type; }
    const std::string& getPrefix() const { return manglePrefix; }
    int getSlot() const { return slot; }

private:
    void load(bool migrate);

    ecs::ComponentType schema;
    ecs::Component params { schema };

    std::filesystem::path path;
    std::string type;
    int version = 0;
    int slot = -1;
    std::optional<size_t> watchId;

    std::string error;
    std::string manglePrefix;
    std::string declarations;
    std::string statements;
};
