#include "shader_plugin.hpp"

#include <format>
#include <unordered_map>

#include "core/core.hpp"
#include "core/fields/field.hpp"
#include "core/shader_plugin/glsl_mangler.hpp"
#include "core/shader_plugin/shader_script.hpp"

ShaderPlugin::ShaderPlugin() {
    registry().push_back(this);
}

ShaderPlugin::~ShaderPlugin() {
    std::erase(registry(), this);
    if (watchId) Core::getFileWatcher().unwatch(*watchId);
}

std::vector<ShaderPlugin*>& ShaderPlugin::registry() {
    static std::vector<ShaderPlugin*> plugins;
    return plugins;
}

void ShaderPlugin::load(bool migrate) {
    manglePrefix = GlslMangler::makePrefix(type, slot);

    error.clear();
    ShaderScript::ParseResult script = ShaderScript::parse(path, type, version);
    if (!script.ok) {
        error = script.error;
        return;
    }

    std::unordered_map<std::string, std::string> seedGlobals;
    for (const Field& field : script.fields)
        seedGlobals[field.getId().filename().string()] = GlslMangler::mangleName(manglePrefix, field.getId().parent_path().string(), field.getId().filename().string());

    GlslMangler::MangleResult mangled = GlslMangler::mangle(script.body, manglePrefix, seedGlobals);
    if (!mangled.ok) {
        error = std::format("{}: {}", path.string(), mangled.error);
        return;
    }
    declarations = std::move(mangled.declarations);
    statements = std::move(mangled.body);

    ecs::ComponentType::Builder builder = ecs::ComponentType::builder(path.string());
    for (Field& field : script.fields) builder.field(field);
    schema = builder.buildDetached();

    ecs::Component newParams(schema);
    if (migrate)
        for (Field& newField : newParams.getFields())
            for (const Field& oldField : params.getFields())
                if (oldField.getId() == newField.getId() && oldField.getType() == newField.getType()) {
                    oldField.dispatch([&](auto v) { newField.set(v); });
                    break;
                }
    params = std::move(newParams);

    Core::markPipelinesDirty();
}

bool ShaderPlugin::parse(const std::filesystem::path& newPath, const std::string& newType, int newVersion, int newSlot) {
    if (newPath == path) return error.empty();

    path = newPath;
    type = newType;
    version = newVersion;
    slot = newSlot;

    load(false);
    if (watchId) Core::getFileWatcher().unwatch(*watchId);
    watchId = Core::getFileWatcher().watch(path, [this] { load(true); });
    return error.empty();
}

std::vector<float> ShaderPlugin::packValues() const {
    std::vector<float> values;
    for (const Field& field : params.getFields()) {
        field.dispatch([&](auto v) {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, bool>) values.push_back(v ? 1.0f : 0.0f);
            else if constexpr (std::is_same_v<V, float>) values.push_back(v);
            else if constexpr (std::is_same_v<V, int>) values.push_back(static_cast<float>(v));
            else for (int i = 0; i < v.length(); i++) values.push_back(static_cast<float>(v[i]));
        });
    }
    return values;
}

std::vector<ShaderPlugin::PluginParam> ShaderPlugin::getParameters() const {
    std::vector<PluginParam> result;
    for (const Field& field : params.getFields()) {
        PluginParam p;
        p.field = &field;
        p.mangled = GlslMangler::mangleName(manglePrefix, field.getId().parent_path().string(), field.getId().filename().string());
        const GlslTypeInfo& typeInfo = *ShaderScript::findGlslType(field.getType());
        p.glslType = typeInfo.name.c_str();
        p.components = typeInfo.components;
        p.isInt = typeInfo.isInt;
        p.isBool = typeInfo.isBool;
        result.push_back(p);
    }
    return result;
}
