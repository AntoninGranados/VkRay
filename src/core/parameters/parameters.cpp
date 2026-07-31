#include "parameters.hpp"

#include <format>
#include <limits>
#include <stdexcept>

#include <glm/glm.hpp>

#include "utils/log.hpp"

void ParameterRegistry::resetAll() {
    for (auto& p : parameters) p->reset();
}

void ParameterRegistry::syncAll() {
    for (auto& p : parameters) p->sync();
}

void ParameterRegistry::setEnumByName(const ParameterPath& path, const std::string& name) {
    auto& p = getParam(path);
    const auto& items = p.getMetadata().enumItems;
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i] == name) {
            p.set(static_cast<int>(i));
            return;
        }
    }
    Log::error("Parameters", std::format("Unknown enum value '{}' for: {}", name, path.string()));
}

Parameter& ParameterRegistry::getParam(const ParameterPath& path) {
    auto it = index.find(path.generic_string());
    if (it == index.end()) throw std::runtime_error("Parameter not found: " + path.generic_string());
    return *it->second;
}

// ===================== print =====================

std::string Parameter::print() const {
    auto constraints = [&](auto mn, auto mx, auto sentinel_lo, auto sentinel_hi) -> std::string {
        bool hasMn = mn > sentinel_lo;
        bool hasMx = mx < sentinel_hi;
        if (hasMn && hasMx) return std::format("{} ... {}", mn, mx);
        if (hasMn)          return std::format("{} ...", mn);
        if (hasMx)          return std::format("... {}", mx);
        return "-";
    };

    const char* p = path.c_str();
    const char* l = getLabel().c_str();
    std::string desc = description.value_or("-");
    const FieldMetadata& metadata = getMetadata();
    const char* r = restartAccumulation ? "✓" : "-";

    switch (type) {
        case FieldType::Bool:
            return std::format("| `{}` | {} | {} | Boolean | {} | - | {} |",
                p, l, desc, getDefault<bool>() ? "true" : "false", r);
        case FieldType::Int: {
            int mn = std::isinf(metadata.min) ? std::numeric_limits<int>::lowest() : (int)metadata.min;
            int mx = std::isinf(metadata.max) ? std::numeric_limits<int>::max() : (int)metadata.max;
            return std::format("| `{}` | {} | {} | Integer | {} | {} | {} |",
                p, l, desc, getDefault<int>(), constraints(mn, mx, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max()), r);
        }
        case FieldType::Float:
            return std::format("| `{}` | {} | {} | Float | {:g} | {} | {} |",
                p, l, desc, getDefault<float>(), constraints(metadata.min, metadata.max, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()), r);
        case FieldType::Enum: {
            int def = getDefault<int>();
            std::string list;
            for (const auto& item : metadata.enumItems)
                list = list.empty() ? std::format("`{}`", item) : std::format("{} • `{}`", list, item);
            return std::format("| `{}` | {} | {} | Enumeration | `{}` | {} | {} |",
                p, l, desc, metadata.enumItems.empty() ? "" : metadata.enumItems[def], list, r);
        }
        case FieldType::Path: {
            std::string defPath = getDefault<std::string>();
            std::string exts;
            for (const auto& e : metadata.pathExtensions) {
                if (!exts.empty()) exts += ", ";
                exts += e.displayName() + " (." + e.ext + ")";
            }
            return std::format("| `{}` | {} | {} | Path | `{}` | {} | {} |",
                p, l, desc, defPath, exts.empty() ? "-" : exts, r);
        }
        case FieldType::IVec2: case FieldType::IVec3: case FieldType::IVec4: {
            int n = type == FieldType::IVec2 ? 2 : type == FieldType::IVec3 ? 3 : 4;
            return std::format("| `{}` | {} | {} | IVec{} | - | - | {} |", p, l, desc, n, r);
        }
        case FieldType::Vec2: case FieldType::Vec3: case FieldType::Vec4: {
            int n = type == FieldType::Vec2 ? 2 : type == FieldType::Vec3 ? 3 : 4;
            return std::format("| `{}` | {} | {} | Vec{} | - | - | {} |", p, l, desc, n, r);
        }
        default: return "";
    }
}
