#include "shader_plugin.hpp"

#include <algorithm>
#include <bit>
#include <format>
#include <string_view>
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

void ShaderPlugin::appendConformances(const ScriptKind& kind, std::vector<VkSmol::ShaderConformance>& conformances) {
    for (size_t i = 0; i < kind.defaults.size(); i++)
        conformances.push_back({kind.defaults[i], kind.dispatchInterface, static_cast<int>(i), ""});

    std::unordered_set<int> seenSlots;
    for (const ShaderPlugin* plugin : registry()) {
        if (plugin->kind != &kind || plugin->structName.empty()) continue;
        if (!seenSlots.insert(plugin->slot).second) continue;
        conformances.push_back({std::format("Script<{}>", plugin->structName), kind.dispatchInterface, plugin->slot,
                                plugin->path->string()});
    }
}

bool ShaderPlugin::appendFields(const std::vector<VkSmol::ShaderField>& shaderFields, const std::string& group,
                                std::vector<Field>& fields) {
    for (const VkSmol::ShaderField& shaderField : shaderFields) {
        const std::string id = group.empty() ? shaderField.name : std::format("{}/{}", group, shaderField.name);
        if (shaderField.type == VkSmol::ShaderFieldType::Struct) {
            if (!appendFields(shaderField.fields, id, fields)) return false;
            continue;
        }

        NumericMeta meta{};
        bool isAnimatable = false;
        bool isTexture = false;
        for (const VkSmol::ShaderAttribute& attribute : shaderField.attributes) {
            const std::vector<float>& arguments = attribute.arguments;
            if (attribute.name == "Texture") {
                isTexture = true;
            } else if (attribute.name == "Range" && arguments.size() == 2) {
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
        const bool isInt =
            shaderField.type == VkSmol::ShaderFieldType::Int || shaderField.type == VkSmol::ShaderFieldType::UInt;
        if (isTexture) {
            if (shaderField.type != VkSmol::ShaderFieldType::Int || shaderField.componentCount != 1) {
                error = std::format("{}: [Texture] needs an int field, '{}' is not", path->string(), id);
                return false;
            }
            fields.push_back(Field::make<ecs::Entity>(id, label, ecs::Entity{}, EntityMeta{.needs = {"texture"}}));
            continue;
        }
        const uint32_t componentCount = shaderField.componentCount;
        if (componentCount < 1 || componentCount > 4 ||
            (shaderField.type == VkSmol::ShaderFieldType::Bool && componentCount > 1)) {
            error = std::format("{}: unsupported type for '{}'", path->string(), id);
            return false;
        }

        if (shaderField.type == VkSmol::ShaderFieldType::Bool) {
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
    std::vector<VkSmol::ShaderStruct> structs;
    for (const std::string& interfaceName : kind->interfaces) {
        try {
            structs = Core::getEngine().getShaderStructs(path->string(), interfaceName);
        } catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
        if (!structs.empty()) break;
    }

    if (structs.size() != 1) {
        error = std::format("{}: expected one struct implementing {}, found {}", path->string(),
                            kind->dispatchInterface, structs.size());
        return false;
    }

    const VkSmol::ShaderStruct& script = structs[0];
    if (!appendFields(script.fields, "", fields)) return false;

    constexpr std::string_view kTextureSuffix = "Texture";
    for (const Field& texture : fields) {
        const std::string textureId = texture.getId().generic_string();
        if (texture.getType() != FieldType::Entity || !textureId.ends_with(kTextureSuffix)) continue;
        const std::string valueId = textureId.substr(0, textureId.size() - kTextureSuffix.size());
        for (Field& value : fields)
            if (value.getId().generic_string() == valueId) value.addCondition({textureId, 0, true});
    }

    for (const VkSmol::ShaderAttribute& attribute : script.attributes) {
        attributes.push_back(attribute.name);
        if (attribute.name == "PassCount" && attribute.arguments.size() == 1)
            passCount = std::max(1, static_cast<int>(attribute.arguments[0]));
    }

    structName = script.name;
    source = Resources::read(*path).value_or("");
    return true;
}

void ShaderPlugin::load(bool migrate) {
    error.clear();
    source.clear();
    structName.clear();
    attributes.clear();
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
                    if (oldField.getType() == FieldType::Entity)
                        newField.set(oldField.get<ecs::Entity>());
                    else
                        oldField.dispatch([&](auto v) { newField.set(v); });
                    break;
                }
    for (Field& newField : newParams.getFields()) {
        const std::string id = newField.getId().generic_string();
        if (const auto pendingEntity = pendingEntities.find(id); pendingEntity != pendingEntities.end())
            newField.set(pendingEntity->second);
        const auto pendingValues = pending.find(id);
        if (pendingValues == pending.end()) continue;
        Field::makeNumeric(newField.getType(), "", "", pendingValues->second).dispatch([&](auto v) {
            newField.set(v);
        });
    }
    pending.clear();
    pendingEntities.clear();
    params = std::move(newParams);

    Core::requestGraphRebuild();
    if (kind != &ScriptTable::kCompositing) Core::markRenderDirty();
}

bool ShaderPlugin::parse(const std::filesystem::path& newPath, const ScriptKind& newKind) {
    if (path && newPath == *path) return error.empty();

    static std::unordered_map<std::filesystem::path, int> slots;

    path = newPath;
    kind = &newKind;
    slot = static_cast<int>(kind->defaults.size()) +
           slots.try_emplace(newPath, static_cast<int>(slots.size())).first->second;

    load(false);
    if (watchId) Core::getFileWatcher().unwatch(*watchId);
    watchId = path->empty() ? std::nullopt : std::optional(Core::getFileWatcher().watch(*path, [this] { load(true); }));
    return error.empty();
}

std::vector<float> ShaderPlugin::packValues(const TextureSlots& textureSlots) const {
    std::vector<float> values;
    for (const Field& field : params.getFields()) {
        if (field.getType() == FieldType::Entity) {
            const auto slot = textureSlots.find(field.get<ecs::Entity>());
            values.push_back(std::bit_cast<float>(slot != textureSlots.end() ? slot->second : 0));
            continue;
        }
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
