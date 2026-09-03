#include "programmable_shader.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "core/core.hpp"
#include "core/field.hpp"
#include "core/scene/scene.hpp"
#include "utils/glsl_dsl_file.hpp"
#include "utils/log.hpp"
#include "utils/string_utils.hpp"

const std::unordered_map<std::string, ProgrammableShader::TypeInfo> ProgrammableShader::typeTable = {
    { "bool",  { FieldType::Bool,  1 } },
    { "float", { FieldType::Float, 1 } },
    { "int",   { FieldType::Int,   1 } },
    { "vec2",  { FieldType::Vec2,  2 } },
    { "vec3",  { FieldType::Vec3,  3 } },
    { "vec4",  { FieldType::Vec4,  4 } },
    { "ivec2", { FieldType::IVec2, 2 } },
    { "ivec3", { FieldType::IVec3, 3 } },
    { "ivec4", { FieldType::IVec4, 4 } },
};

ProgrammableShader::ProgrammableShader() {
    Scene::getProgrammableShaders().push_back(this);
}

ProgrammableShader::~ProgrammableShader() {
    std::erase(Scene::getProgrammableShaders(), this);
}

std::optional<ecs::ComponentField> ProgrammableShader::parseParam(const std::string& line, const std::filesystem::path& path, int lineNumber) {
    std::string location = std::format("{}:{}", path.string(), lineNumber);

    std::vector<std::string> sections = split(trim(line.substr(7)), ':');
    std::vector<std::string> declaration = split(sections[0], '=');

    std::istringstream iss(declaration[0]);
    std::string typeName, name;
    if (!(iss >> typeName >> name)) {
        Log::warn("ProgrammableShader", std::format("{}: expected '#param <type> <name>'", location));
        return std::nullopt;
    }

    auto typeIt = typeTable.find(typeName);
    if (typeIt == typeTable.end()) {
        Log::warn("ProgrammableShader", std::format("{}: unknown type '{}'", location, typeName));
        return std::nullopt;
    }
    const TypeInfo& typeInfo = typeIt->second;

    std::vector<float> values;
    if (declaration.size() > 1) {
        if (typeInfo.fieldType == FieldType::Bool) {
            values.push_back(trim(declaration[1]) == "true" ? 1.0f : 0.0f);
        } else {
            values = parseNumbers(trim(declaration[1]));
            if (values.size() != 1 && (int)values.size() != typeInfo.components) {
                Log::warn("ProgrammableShader", std::format("{}: default value does not match type '{}'", location, typeName));
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
                    Log::warn("ProgrammableShader", std::format("{}: malformed '{}'", location, metaToken));
                    return std::nullopt;
                }
                (isMin ? meta.min : meta.max) = value[0];
            } else {
                Log::warn("ProgrammableShader", std::format("{}: unknown metadata '{}'", location, metaToken));
                return std::nullopt;
            }
        }
    }

    ecs::ComponentField field;
    if (typeInfo.fieldType == FieldType::Bool) {
        static_cast<Field&>(field) = Field::make<bool>(name, camelCaseToLabel(name), static_cast<bool>(values[0]));
    } else {
        static_cast<Field&>(field) = Field::makeNumeric(typeInfo.fieldType, name, camelCaseToLabel(name), values, meta);
    }
    field.setAnimatable(isAnimatable);
    return field;
}

ProgrammableShader::TypeSpec ProgrammableShader::typeSpecFor(FieldType type) {
    switch (type) {
        case FieldType::Bool:  return { "bool",  1, false, true  };
        case FieldType::Float: return { "float", 1, false, false };
        case FieldType::Int:   return { "int",   1, true,  false };
        case FieldType::Vec2:  return { "vec2",  2, false, false };
        case FieldType::Vec3:  return { "vec3",  3, false, false };
        case FieldType::Vec4:  return { "vec4",  4, false, false };
        case FieldType::IVec2: return { "ivec2", 2, true,  false };
        case FieldType::IVec3: return { "ivec3", 3, true,  false };
        default:               return { "ivec4", 4, true,  false };
    }
}

std::string ProgrammableShader::mangledPrefix() const {
    return std::format("_{}{}_", KIND, slot);
}

std::string ProgrammableShader::declareGlobal(const ecs::ComponentField& field) const {
    const TypeSpec spec = typeSpecFor(field.getType());
    return std::format("{} {}{};", spec.glsl, mangledPrefix(), field.getId());
}

std::string ProgrammableShader::assignParam(const ecs::ComponentField& field, int& offset) const {
    const TypeSpec spec = typeSpecFor(field.getType());
    const std::string mangled = mangledPrefix() + field.getId();

    std::string args;
    for (int i = 0; i < spec.components; i++) {
        if (i > 0) args += ", ";
        std::string value = std::format("materialParams.values[base+{}]", offset + i);
        if (spec.isInt) {
            args += std::format("int({})", value);
        } else if (spec.isBool) {
            args += std::format("bool({})", value);
        } else {
            args += value;
        }
    }

    std::string line = spec.components == 1
        ? std::format("{} = {};", mangled, args)
        : std::format("{} = {}({});", mangled, spec.glsl, args);

    offset += spec.components;
    return line;
}

std::vector<float> ProgrammableShader::packValues() const {
    std::vector<float> values;
    for (const ecs::ComponentField& field : params.getFields()) {
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

std::string ProgrammableShader::generateGlobalDecls() const {
    std::string decls;
    for (const ecs::ComponentField& field : params.getFields())
        decls += declareGlobal(field) + "\n";
    return decls;
}

std::string ProgrammableShader::generateParamAssignments() const {
    std::string assignments;
    int offset = 0;
    for (const ecs::ComponentField& field : params.getFields())
        assignments += assignParam(field, offset) + "\n";
    return assignments;
}

void ProgrammableShader::load(bool migrate) {
    slot = slotForPath(path);
    if (!GlslDslFile::parse(path, { .manglePrefix = mangledPrefix(), .expectedVersion = static_cast<int>(kShaderVersion) }))
        return;

    std::vector<ecs::ComponentField> newFields;
    const std::vector<std::string>& paramLines = getParamLines();
    for (size_t i = 0; i < paramLines.size(); i++)
        if (std::optional<ecs::ComponentField> field = parseParam(paramLines[i], path, static_cast<int>(i) + 1))
            newFields.push_back(std::move(*field));

    ecs::ComponentType::Builder builder = ecs::ComponentType::builder(path.string());
    for (ecs::ComponentField& f : newFields) builder.field(f);
    schema = builder.buildDetached();

    ecs::Component newParams(schema);
    if (migrate)
        for (ecs::ComponentField& newField : newParams.getFields())
            for (const ecs::ComponentField& oldField : params.getFields())
                if (oldField.getId() == newField.getId() && oldField.getType() == newField.getType()) {
                    oldField.dispatch([&](auto v) { newField.set(v); });
                    break;
                }
    params = std::move(newParams);

    Core::markPipelinesDirty();
}

bool ProgrammableShader::reload(const std::filesystem::path& newPath) {
    path = newPath;
    load(false);
    Core::getFileWatcher().watch(path, [this] { load(true); });
    return getError().empty();
}

bool ProgrammableShader::parse(const std::filesystem::path& newPath) {
    if (newPath == path)
        return getError().empty();
    return reload(newPath);
}

int ProgrammableShader::slotForPath(const std::filesystem::path& path) {
    static std::unordered_map<std::filesystem::path, int> slots;
    const auto [it, inserted] = slots.try_emplace(path, static_cast<int>(slots.size()));
    return it->second;
}

void ProgrammableShader::generateDispatch() {
    std::string functions;
    std::string cases;
    std::unordered_set<int> emittedSlots;

    for (ProgrammableShader* shader : Scene::getProgrammableShaders()) {
        if (!shader->getError().empty() || shader->getMainBody().empty()) continue;
        if (!emittedSlots.insert(shader->slot).second) continue;

        const std::string funcName = shader->mangledPrefix() + "programmable";
        functions += std::format(
            "// ============================= {}{} =============================\n"
            "{}"
            "{}"
            "ResolvedMaterial {} (int base, vec3 pos, vec2 uv, vec3 normal, vec3 wo, RngState rng, inout vec3 new_normal) {{\n"
            "{}"
            "ResolvedMaterial mat;\n"
            "{}"
            "return mat;\n"
            "}}\n\n",
            KIND, shader->slot, shader->generateGlobalDecls(), shader->getOutBody(), funcName, shader->generateParamAssignments(), shader->getMainBody()
        );
        cases += std::format("        case {}: result = {} (base, pos, uv, normal, wo, rng, new_normal); break;\n", shader->slot, funcName);
    }

    std::string content = std::format(
        "// WARN: AUTO-GENERATED by ProgrammableShader::generateDispatch, do not edit by hand.\n"
        "// Source: the programmable material scripts loaded into the scene (see assets/materials/).\n"
        "{}\n"
        "ResolvedMaterial dispatchProgrammable(in Material mat, inout Hit hit, in vec3 wo, inout RngState rng) {{\n"
        "    vec3 pos    = hit.p;\n"
        "    vec3 normal = hit.normal;\n"
        "    vec2 uv     = hit.uv;\n"
        "    vec3 new_normal = normal;\n"
        "    int base = int(mat.base) + 1;\n"
        "    ResolvedMaterial result = DEFAULT_MATERIAL;\n"
        "    switch (int(materialParams.values[mat.base])) {{\n"
        "{}"
        "    }}\n"
        "    hit.normal = new_normal;\n"
        "    return result;\n"
        "}}\n",
        functions, cases
    );

    std::filesystem::path outputPath = "./src/shaders/core/materials/generated/programmable_dispatch.glsl";
    std::error_code ec;
    std::filesystem::create_directories(outputPath.parent_path(), ec);

    std::ifstream existing(outputPath);
    std::stringstream existingBuffer;
    existingBuffer << existing.rdbuf();
    if (existingBuffer.str() == content) return;

    std::ofstream(outputPath) << content;
}
