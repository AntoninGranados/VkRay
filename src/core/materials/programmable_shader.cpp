#include "programmable_shader.hpp"

#include <format>
#include <fstream>
#include <sstream>

#include "core/ecs/components/component_type.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"

const std::unordered_map<std::string, ProgrammableShader::TypeInfo> ProgrammableShader::typeTable = {
    { "float", { FieldType::Float, 1 } },
    { "int",   { FieldType::Int,   1 } },
    { "vec2",  { FieldType::Vec2,  2 } },
    { "vec3",  { FieldType::Vec3,  3 } },
    { "vec4",  { FieldType::Vec4,  4 } },
    { "ivec2", { FieldType::IVec2, 2 } },
    { "ivec3", { FieldType::IVec3, 3 } },
    { "ivec4", { FieldType::IVec4, 4 } },
};

std::optional<Field> ProgrammableShader::parseParam(const std::string& statement) {
    std::istringstream iss(statement);
    std::string typeName, name, equalsSign;
    if (!(iss >> typeName >> name >> equalsSign) || equalsSign != "=") {
        Log::warn("ProgrammableShader", std::format("Could not parse param declaration: [{}]", statement));
        return std::nullopt;
    }

    std::string rest;
    std::getline(iss, rest);
    size_t colon = rest.find(':');
    std::string defaultExpr = trim(colon == std::string::npos ? rest : rest.substr(0, colon));
    std::string metaText = colon == std::string::npos ? "" : trim(rest.substr(colon + 1));

    auto typeIt = typeTable.find(typeName);
    if (typeIt == typeTable.end()) {
        Log::warn("ProgrammableShader", std::format("Unsupported param type: [{}]", typeName));
        return std::nullopt;
    }
    const TypeInfo& typeInfo = typeIt->second;

    NumericMeta meta{};
    for (const std::string& token : split(metaText, ',')) {
        if (token.empty()) continue;
        bool isMin = token.starts_with("min(");
        bool isMax = token.starts_with("max(");
        if (token == "color") {
            meta.color = true;
        } else if (isMin || isMax) {
            std::vector<float> value = parseNumbers(token);
            if (value.empty()) {
                Log::warn("ProgrammableShader", std::format("Could not parse metadata for param [{}]: [{}]", name, token));
                return std::nullopt;
            }
            (isMin ? meta.min : meta.max) = value[0];
        } else {
            Log::warn("ProgrammableShader", std::format("Unsupported metadata for param [{}]: [{}]", name, token));
            return std::nullopt;
        }
    }

    std::vector<float> values = parseNumbers(defaultExpr);
    if (values.size() != 1 && (int)values.size() != typeInfo.components) {
        Log::warn("ProgrammableShader", std::format("Could not parse default value for param [{}]: [{}]", name, defaultExpr));
        return std::nullopt;
    }

    return Field::makeNumeric(typeInfo.fieldType, name, ecs::ComponentType::deriveLabel(name), values, meta);
}

bool ProgrammableShader::parse(const std::filesystem::path& path) {
    error.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        error = std::format("Could not open file [{}]", path.string());
        return false;
    }

    std::vector<Field> newParams;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (!line.starts_with("#param ")) continue;
        if (std::optional<Field> field = parseParam(line.substr(7)))
            newParams.push_back(std::move(*field));
    }

    for (Field& field : newParams)
        for (const Field& old : params)
            if (old.getId() == field.getId() && old.getType() == field.getType()) {
                old.dispatch([&](auto v) { field.set(v); });
                break;
            }
    params = std::move(newParams);

    return true;
}
