#include "shader_script.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <sstream>
#include <unordered_set>

#include "core/shader_plugin/shader_source_map.hpp"

#include "utils/log.hpp"
#include "utils/resources.hpp"
#include "utils/string_utils.hpp"

const std::vector<GlslTypeInfo> ShaderScript::glslTypes = {
    {"bool", FieldType::Bool, 1, false, true},   {"float", FieldType::Float, 1, false, false},
    {"int", FieldType::Int, 1, true, false},     {"vec2", FieldType::Vec2, 2, false, false},
    {"vec3", FieldType::Vec3, 3, false, false},  {"vec4", FieldType::Vec4, 4, false, false},
    {"ivec2", FieldType::IVec2, 2, true, false}, {"ivec3", FieldType::IVec3, 3, true, false},
    {"ivec4", FieldType::IVec4, 4, true, false},
};

const GlslTypeInfo* ShaderScript::findGlslType(const std::string& name) {
    for (const GlslTypeInfo& info : glslTypes)
        if (info.name == name) return &info;
    return nullptr;
}

const GlslTypeInfo* ShaderScript::findGlslType(FieldType fieldType) {
    for (const GlslTypeInfo& info : glslTypes)
        if (info.fieldType == fieldType) return &info;
    return nullptr;
}

namespace {
int extractDirectiveInt(const std::string& line, const std::string& key, int fallback) {
    const size_t pos = line.find(key + "(");
    if (pos == std::string::npos) return fallback;
    const std::vector<float> values = parseNumbers(line.substr(pos));
    return values.empty() ? fallback : static_cast<int>(values[0]);
}
} // namespace

std::optional<Field> ShaderScript::parseParam(const std::string& group, const std::string& line,
                                              const std::filesystem::path& path, int lineNumber) {
    std::string location = std::format("{}:{}", path.string(), lineNumber);

    std::vector<std::string> sections = split(trim(line.substr(7)), ':');
    std::vector<std::string> declaration = split(sections[0], '=');

    std::istringstream iss(declaration[0]);
    std::string typeName, name;
    if (!(iss >> typeName >> name)) {
        Log::warn("ShaderScript", std::format("{}: expected '#param <type> <name>'", location));
        return std::nullopt;
    }

    const GlslTypeInfo* typeInfoPtr = findGlslType(typeName);
    if (!typeInfoPtr) {
        Log::warn("ShaderScript", std::format("{}: unknown type '{}'", location, typeName));
        return std::nullopt;
    }
    const GlslTypeInfo& typeInfo = *typeInfoPtr;

    std::vector<float> values;
    if (declaration.size() > 1) {
        if (typeInfo.fieldType == FieldType::Bool) {
            values.push_back(trim(declaration[1]) == "true" ? 1.0f : 0.0f);
        } else {
            values = parseNumbers(trim(declaration[1]));
            if (values.size() != 1 && (int)values.size() != typeInfo.components) {
                Log::warn("ShaderScript",
                          std::format("{}: default value does not match type '{}'", location, typeName));
                return std::nullopt;
            }
        }
    } else {
        values.assign(typeInfo.components, 0.0f);
    }

    NumericMeta meta{};
    bool isAnimatable = false;
    if (sections.size() > 1) {
        for (const std::string& token : split(sections[1], ',')) {
            std::string metaToken = trim(token);
            if (metaToken.empty()) continue;
            bool isMin = metaToken.starts_with("min(");
            bool isMax = metaToken.starts_with("max(");
            if (metaToken == "color") {
                meta.color = true;
            } else if (metaToken == "animatable") {
                isAnimatable = true;
            } else if (isMin || isMax) {
                std::vector<float> value = parseNumbers(metaToken);
                if (value.empty()) {
                    Log::warn("ShaderScript", std::format("{}: malformed '{}'", location, metaToken));
                    return std::nullopt;
                }
                (isMin ? meta.min : meta.max) = value[0];
            } else {
                Log::warn("ShaderScript", std::format("{}: unknown metadata '{}'", location, metaToken));
                return std::nullopt;
            }
        }
    }

    Field field;
    FieldPath fieldPath = group.empty() ? name : std::format("{}/{}", group, name);
    if (typeInfo.fieldType == FieldType::Bool) {
        static_cast<Field&>(field) = Field::make<bool>(fieldPath, camelCaseToLabel(name), static_cast<bool>(values[0]));
    } else {
        static_cast<Field&>(field) =
            Field::makeNumeric(typeInfo.fieldType, fieldPath, camelCaseToLabel(name), values, meta);
    }
    field.setAnimatable(isAnimatable);
    return field;
}

ShaderScript::ParseResult ShaderScript::parse(const std::filesystem::path& path, const std::string& type, int version) {
    ParseResult result;

    const std::optional<std::string> source = Resources::read(path);
    if (!source) {
        result.error = std::format("Could not open file [{}]", path.string());
        return result;
    }

    bool hasVersion = false;
    std::string group = "";
    std::vector<std::pair<std::string, std::string>> paramLines;
    const std::string versionDirective = "#" + type;

    const std::vector<std::string> lines = split(*source, '\n');
    for (size_t lineIdx = 0; lineIdx < lines.size(); lineIdx++) {
        std::string trimmed = trim(lines[lineIdx]);
        const size_t commentPos = trimmed.find("//");
        if (commentPos != trimmed.npos) trimmed = trim(trimmed.substr(0, commentPos));
        if (trimmed.empty()) continue;

        if (trimmed.starts_with("#param ")) {
            paramLines.push_back({group, trimmed});
        } else if (trimmed.starts_with("#group ")) {
            trimmed = trimmed.substr(trimmed.find("\"") + 1);
            group = trimmed.substr(0, trimmed.find("\""));
        } else if (trimmed.starts_with(versionDirective) && trimmed.find(':') != std::string::npos) {
            hasVersion = extractDirectiveInt(trimmed, "version", -1) == version;
            result.passCount = std::max(1, extractDirectiveInt(trimmed, "pass_count", 1));
        } else {
            result.body += trimmed + "\n";
            result.bodyLineMarkers.push_back(
                trimmed.ends_with('\\') ? std::string() : ShaderSourceMap::marker(path, static_cast<int>(lineIdx) + 1));
        }
    }

    if (!hasVersion) {
        result.error = std::format("{}: expected '{}: version({})'", path.string(), versionDirective, version);
        return result;
    }

    std::unordered_set<FieldPath> seenIds;
    for (size_t i = 0; i < paramLines.size(); i++) {
        std::optional<Field> field =
            parseParam(paramLines[i].first, paramLines[i].second, path, static_cast<int>(i) + 1);
        if (!field) continue;

        if (!seenIds.insert(field->getId()).second) {
            Log::warn("ShaderScript",
                      std::format("{}:{}: duplicate param '{}'", path.string(), i + 1, field->getId().string()));
            continue;
        }
        result.fields.push_back(std::move(*field));
    }

    result.ok = true;
    return result;
}
