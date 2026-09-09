#include "field_serializer.hpp"

#include <cmath>
#include <filesystem>
#include <format>
#include <utility>

#include "utils/log.hpp"

using json = nlohmann::ordered_json;

json fieldValueToJson(const FieldValue& f) {
    switch (f.getType()) {
        case FieldType::Bool:   return f.get<bool>();
        case FieldType::Int:
        case FieldType::Enum:   return f.get<int>();
        case FieldType::Float:  return f.get<float>();
        case FieldType::IVec2: { auto v = f.get<glm::ivec2>(); return json{v.x, v.y}; }
        case FieldType::IVec3: { auto v = f.get<glm::ivec3>(); return json{v.x, v.y, v.z}; }
        case FieldType::IVec4: { auto v = f.get<glm::ivec4>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Vec2:  { auto v = f.get<glm::vec2>(); return json{v.x, v.y}; }
        case FieldType::Vec3:  { auto v = f.get<glm::vec3>(); return json{v.x, v.y, v.z}; }
        case FieldType::Vec4:  { auto v = f.get<glm::vec4>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Quat:  { auto v = f.get<glm::quat>(); return json{v.x, v.y, v.z, v.w}; }
        case FieldType::Entity: return nullptr;
        case FieldType::String: return trimmed(f.get<std::string>());
        case FieldType::Path:   return f.get<std::filesystem::path>().string();
    }
    std::unreachable();
}

FieldValue fieldValueFromJson(const json& v, FieldType type) {
    switch (type) {
        case FieldType::Bool:   return FieldValue::make(v.get<bool>());
        case FieldType::Int:
        case FieldType::Enum:   return FieldValue::make(v.get<int>());
        case FieldType::Float:  return FieldValue::make(v.get<float>());
        case FieldType::IVec2:  return FieldValue::make(glm::ivec2{v[0].get<int>(), v[1].get<int>()});
        case FieldType::IVec3:  return FieldValue::make(glm::ivec3{v[0].get<int>(), v[1].get<int>(), v[2].get<int>()});
        case FieldType::IVec4:  return FieldValue::make(glm::ivec4{v[0].get<int>(), v[1].get<int>(), v[2].get<int>(), v[3].get<int>()});
        case FieldType::Vec2:   return FieldValue::make(glm::vec2{v[0].get<float>(), v[1].get<float>()});
        case FieldType::Vec3:   return FieldValue::make(glm::vec3{v[0].get<float>(), v[1].get<float>(), v[2].get<float>()});
        case FieldType::Vec4:   return FieldValue::make(glm::vec4{v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>()});
        case FieldType::String: return FieldValue::make(v.get<std::string>());
        case FieldType::Path:   return FieldValue::make(std::filesystem::path(v.get<std::string>()));
        default:
            Log::error("FieldSerializer", std::format("Unsupported field type: {}", static_cast<int>(type)));
            return FieldValue::make(false);
    }
}

void applyField(const json& j, Field& f, const ResolveCtx& ctx) {
    switch (f.getType()) {
        case FieldType::Bool:   f.set<bool>(j.get<bool>()); break;
        case FieldType::Int:
        case FieldType::Enum:   f.set<int>(j.is_number_integer() ? j.get<int>() : static_cast<int>(std::round(resolveFloat(j, ctx)))); break;
        case FieldType::IVec2:  if (expectArray(j, 2, f.getId().string())) f.set(fieldValueFromJson(j, FieldType::IVec2).get<glm::ivec2>()); break;
        case FieldType::IVec3:  if (expectArray(j, 3, f.getId().string())) f.set(fieldValueFromJson(j, FieldType::IVec3).get<glm::ivec3>()); break;
        case FieldType::IVec4:  if (expectArray(j, 4, f.getId().string())) f.set(fieldValueFromJson(j, FieldType::IVec4).get<glm::ivec4>()); break;
        case FieldType::Float:  f.set<float>(resolveFloat(j, ctx)); break;
        case FieldType::Vec2:   f.set<glm::vec2>(resolveVec2(j, ctx)); break;
        case FieldType::Vec3:   f.set<glm::vec3>(resolveVec3(j, ctx)); break;
        case FieldType::Vec4:   if (expectArray(j, 4, f.getId().string())) f.set<glm::vec4>({resolveFloat(j[0], ctx), resolveFloat(j[1], ctx), resolveFloat(j[2], ctx), resolveFloat(j[3], ctx)}); break;
        case FieldType::Quat:   if (expectArray(j, 4, f.getId().string())) f.set<glm::quat>(glm::quat(j[3].get<float>(), j[0].get<float>(), j[1].get<float>(), j[2].get<float>())); break;
        case FieldType::Entity: break;
        case FieldType::String: f.set<std::string>(resolveTemplate(j.get<std::string>(), ctx)); break;
        case FieldType::Path:   f.set<std::filesystem::path>(j.get<std::string>()); break;
    }
}

std::optional<FieldValue> inferFieldValueFromJson(const json& v) {
    if (v.is_boolean()) return FieldValue::make(v.get<bool>());
    if (v.is_number_integer()) return FieldValue::make(v.get<int>());
    if (v.is_number_float()) return FieldValue::make(v.get<float>());
    if (v.is_string()) return FieldValue::make(v.get<std::string>());
    if (v.is_array() && (v.size() == 2 || v.size() == 3 || v.size() == 4)) {
        const bool isFloat = v[0].is_number_float();
        switch (v.size()) {
            case 2:  return fieldValueFromJson(v, isFloat ? FieldType::Vec2 : FieldType::IVec2);
            case 3:  return fieldValueFromJson(v, isFloat ? FieldType::Vec3 : FieldType::IVec3);
            default: return fieldValueFromJson(v, isFloat ? FieldType::Vec4 : FieldType::IVec4);
        }
    }
    return std::nullopt;
}
