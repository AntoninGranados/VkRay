#include "component_serializer.hpp"

#include <cmath>
#include <format>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "core/ecs/components/component_type.hpp"

static std::string fieldTypeName(FieldType type) {
    switch (type) {
        case FieldType::Bool:   return "bool";
        case FieldType::Int:    return "int";
        case FieldType::Enum:   return "enum";
        case FieldType::Float:  return "float";
        case FieldType::IVec2:  return "ivec2";
        case FieldType::IVec3:  return "ivec3";
        case FieldType::IVec4:  return "ivec4";
        case FieldType::Vec2:   return "vec2";
        case FieldType::Vec3:   return "vec3";
        case FieldType::Vec4:   return "vec4";
        case FieldType::Quat:   return "quat";
        case FieldType::Entity: return "entity";
        case FieldType::String: return "string";
        case FieldType::Path:   return "path";
    }
    std::unreachable();
}

static std::string fieldDefaultStr(const Field& f) {
    switch (f.getType()) {
        case FieldType::Bool:  return f.getDefault<bool>() ? "true" : "false";
        case FieldType::Int:
        case FieldType::Enum:  return std::to_string(f.getDefault<int>());
        case FieldType::Float: return std::format("{:g}", f.getDefault<float>());
        case FieldType::IVec2: { auto v = f.getDefault<glm::ivec2>(); return std::format("[{}, {}]", v.x, v.y); }
        case FieldType::IVec3: { auto v = f.getDefault<glm::ivec3>(); return std::format("[{}, {}, {}]", v.x, v.y, v.z); }
        case FieldType::IVec4: { auto v = f.getDefault<glm::ivec4>(); return std::format("[{}, {}, {}, {}]", v.x, v.y, v.z, v.w); }
        case FieldType::Vec2:  { auto v = f.getDefault<glm::vec2>(); return std::format("[{:g}, {:g}]", v.x, v.y); }
        case FieldType::Vec3:  { auto v = f.getDefault<glm::vec3>(); return std::format("[{:g}, {:g}, {:g}]", v.x, v.y, v.z); }
        case FieldType::Vec4:  { auto v = f.getDefault<glm::vec4>(); return std::format("[{:g}, {:g}, {:g}, {:g}]", v.x, v.y, v.z, v.w); }
        default:               return "";
    }
}

static std::string fieldConstraintStr(const FieldMetadata& m) {
    const auto* num = std::get_if<NumericMeta>(&m);
    if (!num) return "";
    const bool hasMin = std::isfinite(num->min);
    const bool hasMax = std::isfinite(num->max);
    if (hasMin && hasMax) return std::format("{:g} ... {:g}", num->min, num->max);
    if (hasMin) return std::format("≥ {:g}", num->min);
    if (hasMax) return std::format("≤ {:g}", num->max);
    return "";
}

void ComponentSerializer::saveDocumentation(std::filesystem::path path) {
    std::ofstream file(path);

    file << "# Components\n\n";
    file << "Components are defined in `src/core/ecs/components.hpp`.\n";

    std::map<std::string, std::vector<const ecs::ComponentType*>> groups;
    for (const auto& type : ecs::ComponentType::all())
        groups[type.getGroup()].push_back(&type);

    for (const auto& [group, types] : groups) {
        file << "\n## " << snakeCaseToLabel(group) << "\n";
        for (const auto* type : types) {
            file << "\n### " << type->getLabel() << "\n";
            if (!type->getDescription().empty())
                file << type->getDescription() << "\n";

            if (!type->getNeeds().empty() || !type->getConflicts().empty()) {
                file << "\n";
                if (!type->getNeeds().empty()) {
                    file << "**Needs:**";
                    for (const auto& n : type->getNeeds()) file << " `" << n << "`";
                    if (!type->getConflicts().empty()) file << " —";
                    else file << "\n";
                }
                if (!type->getConflicts().empty()) {
                    file << " **Conflicts:**";
                    for (const auto& c : type->getConflicts()) file << " `" << c << "`";
                    file << "\n";
                }
            }

            std::vector<const ecs::ComponentField*> publicFields;
            for (const auto& f : type->getFields())
                publicFields.push_back(&f);

            if (!publicFields.empty()) {
                file << "\n| Field | Type | Default | Constraints | Animatable |\n";
                file <<   "|-------|------|---------|-------------|------------|\n";
                for (const auto* f : publicFields) {
                    file << std::format("| `{}` | {} | {} | {} | {} |\n",
                        f->getId(), fieldTypeName(f->getType()), fieldDefaultStr(*f),
                        fieldConstraintStr(f->getMetadata()), f->isAnimatable() ? "yes" : "no");
                }
            }
        }
    }
}
