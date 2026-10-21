#include "shader_plugin.hpp"

#include <algorithm>
#include <bit>
#include <format>
#include <unordered_map>
#include <unordered_set>

#include "VkSmol/render/shader.hpp"

#include "core/core.hpp"
#include "core/fields/field.hpp"
#include "core/render/script_table.hpp"

#include "utils/log.hpp"
#include "utils/resources.hpp"
#include "utils/string_utils.hpp"

ShaderPlugin::ShaderPlugin() { registry().push_back(this); }

ShaderPlugin::~ShaderPlugin() {
    std::erase(registry(), this);
    if (watchId) Core::getFileWatcher().unwatch(*watchId);
}

std::vector<ShaderPlugin*>& ShaderPlugin::registry() {
    static std::vector<ShaderPlugin*> plugins;
    return plugins;
}

void ShaderPlugin::appendConformances(const ScriptKind& kind, std::vector<ShaderConformance>& conformances) {
    for (size_t i = 0; i < kind.builtins.size(); i++)
        conformances.push_back({kind.builtins[i], kind.interfaceName, static_cast<int>(i), ""});

    std::unordered_set<int> seenSlots;
    for (const ShaderPlugin* plugin : registry()) {
        if (plugin->kind != &kind || plugin->structName.empty()) continue;
        if (!seenSlots.insert(plugin->slot).second) continue;
        conformances.push_back({std::format("{}<{}>", kind.wrapperName, plugin->structName), kind.interfaceName,
                                plugin->slot, plugin->path->string()});
    }
}

bool ShaderPlugin::appendFields(const std::vector<ShaderField>& shaderFields, const std::string& group,
                                std::vector<Field>& fields) {
    for (const ShaderField& shaderField : shaderFields) {
        const std::string id = group.empty() ? shaderField.name : std::format("{}/{}", group, shaderField.name);
        if (shaderField.type == ShaderFieldType::Struct) {
            if (!appendFields(shaderField.fields, id, fields)) return false;
            continue;
        }

        NumericMeta meta{};
        bool isAnimatable = false;
        for (const ShaderAttribute& attribute : shaderField.attributes) {
            const std::vector<float>& arguments = attribute.arguments;
            if (attribute.name == "Range" && arguments.size() == 2) {
                meta.min = arguments[0];
                meta.max = arguments[1];
            } else if (attribute.name == "Min" && arguments.size() == 1) {
                meta.min = arguments[0];
            } else if (attribute.name == "Max" && arguments.size() == 1) {
                meta.max = arguments[0];
            } else if (attribute.name == "Color") {
                meta.color = true;
            } else if (attribute.name == "Animatable") {
                isAnimatable = true;
            } else {
                error = std::format("{}: unknown attribute '{}' on '{}'", path->string(), attribute.name, id);
                return false;
            }
        }

        const std::string label = camelCaseToLabel(shaderField.name);
        const bool isInt = shaderField.type == ShaderFieldType::Int || shaderField.type == ShaderFieldType::UInt;
        const uint32_t componentCount = shaderField.componentCount;
        if (componentCount < 1 || componentCount > 4 ||
            (shaderField.type == ShaderFieldType::Bool && componentCount > 1)) {
            error = std::format("{}: unsupported type for '{}'", path->string(), id);
            return false;
        }

        if (shaderField.type == ShaderFieldType::Bool) {
            fields.push_back(Field::make<bool>(id, label, shaderField.defaultValue[0] != 0));
        } else {
            static constexpr FieldType floatTypes[] = {FieldType::Float, FieldType::Vec2, FieldType::Vec3,
                                                       FieldType::Vec4};
            static constexpr FieldType intTypes[] = {FieldType::Int, FieldType::IVec2, FieldType::IVec3,
                                                     FieldType::IVec4};
            std::vector<float> values;
            for (const uint32_t word : shaderField.defaultValue)
                values.push_back(isInt ? static_cast<float>(std::bit_cast<int32_t>(word)) : std::bit_cast<float>(word));
            fields.push_back(
                Field::makeNumeric((isInt ? intTypes : floatTypes)[componentCount - 1], id, label, values, meta));
        }
        fields.back().setAnimatable(isAnimatable);
    }
    return true;
}

bool ShaderPlugin::reflect(std::vector<Field>& fields) {
    std::vector<ShaderStruct> structs;
    try {
        structs = Core::getEngine().getShaderStructs(path->string(), kind->interfaceName);
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }

    if (structs.size() != 1) {
        error = std::format("{}: expected one struct implementing {}, found {}", path->string(), kind->interfaceName,
                            structs.size());
        return false;
    }

    const ShaderStruct& script = structs[0];
    if (!appendFields(script.fields, "", fields)) return false;

    for (const ShaderAttribute& attribute : script.attributes)
        if (attribute.name == "PassCount" && attribute.arguments.size() == 1)
            passCount = std::max(1, static_cast<int>(attribute.arguments[0]));

    structName = script.name;
    source = Resources::read(*path).value_or("");
    return true;
}

void ShaderPlugin::load(bool migrate) {
    error.clear();
    source.clear();
    structName.clear();
    passCount = 1;

    std::vector<Field> fields;
    if (!reflect(fields)) {
        Log::error("ShaderPlugin", error);
        return;
    }

    ecs::ComponentType::Builder builder = ecs::ComponentType::builder(path->string());
    for (Field& field : fields) builder.field(field);
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

    Core::requestGraphRebuild();
    if (kind != &ScriptTable::kCompositing) Core::markRenderDirty();
}

bool ShaderPlugin::parse(const std::filesystem::path& newPath, const ScriptKind& newKind) {
    if (path && newPath == *path) return error.empty();

    static std::unordered_map<std::filesystem::path, int> slots;

    path = newPath;
    kind = &newKind;
    slot = static_cast<int>(kind->builtins.size()) +
           slots.try_emplace(newPath, static_cast<int>(slots.size())).first->second;

    load(false);
    if (watchId) Core::getFileWatcher().unwatch(*watchId);
    watchId = path->empty() ? std::nullopt : std::optional(Core::getFileWatcher().watch(*path, [this] { load(true); }));
    return error.empty();
}

std::vector<float> ShaderPlugin::packValues() const {
    std::vector<float> values;
    for (const Field& field : params.getFields()) {
        field.dispatch([&](auto v) {
            using V = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<V, bool>)
                values.push_back(std::bit_cast<float>(v ? 1 : 0));
            else if constexpr (std::is_same_v<V, float>)
                values.push_back(v);
            else if constexpr (std::is_same_v<V, int>)
                values.push_back(std::bit_cast<float>(v));
            else
                for (int i = 0; i < v.length(); i++) {
                    if constexpr (std::is_integral_v<std::decay_t<decltype(v[i])>>)
                        values.push_back(std::bit_cast<float>(static_cast<int>(v[i])));
                    else
                        values.push_back(static_cast<float>(v[i]));
                }
        });
    }
    return values;
}
