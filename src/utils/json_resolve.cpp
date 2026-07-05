#include "utils/json_resolve.hpp"

#include <format>
#include <sstream>
#include <vector>

using json = nlohmann::ordered_json;

static float randRange(float lo, float hi, const ResolveCtx& ctx) {
    return lo + std::uniform_real_distribution<float>(0.0f, 1.0f)(ctx.rng) * (hi - lo);
}

static float compOf(const json& v, int i) {
    return (v.is_array() ? v[i] : v).get<float>();
}

static float lerpAxis(float from, float to, const std::string& axis, const ResolveCtx& ctx) {
    const auto it = ctx.tokens.find(axis);
    if (it != ctx.tokens.end() && it->second.second > 1) {
        const float t = static_cast<float>(it->second.first) / static_cast<float>(it->second.second - 1);
        return from + t * (to - from);
    }
    return from;
}

float resolveFloat(const json& v, const ResolveCtx& ctx) {
    if (v.is_number()) return v.get<float>();
    if (!v.is_object()) return 0.0f;
    if (v.contains("rand")) {
        const auto& r = v["rand"];
        return randRange(r["min"].get<float>(), r["max"].get<float>(), ctx);
    }
    if (v.contains("lerp")) {
        const auto& l = v["lerp"];
        const std::string axis = l.value("axis", "col");
        return lerpAxis(l["from"].get<float>(), l["to"].get<float>(), axis, ctx);
    }
    return 0.0f;
}

glm::vec2 resolveVec2(const json& v, const ResolveCtx& ctx) {
    if (v.is_array() && v.size() >= 2)
        return { resolveFloat(v[0], ctx), resolveFloat(v[1], ctx) };
    if (v.is_object()) {
        if (v.contains("rand")) {
            const auto& r = v["rand"];
            return { randRange(compOf(r["min"],0), compOf(r["max"],0), ctx),
                     randRange(compOf(r["min"],1), compOf(r["max"],1), ctx) };
        }
        if (v.contains("lerp")) {
            const auto& l = v["lerp"];
            const std::string axis = l.value("axis", "col");
            const glm::vec2 from = resolveVec2(l["from"], ctx), to = resolveVec2(l["to"], ctx);
            return {
                lerpAxis(from.x, to.x, axis, ctx),
                lerpAxis(from.y, to.y, axis, ctx)
            };
        }
    }
    return {};
}

glm::vec3 resolveVec3(const json& v, const ResolveCtx& ctx) {
    if (v.is_array() && v.size() >= 3)
        return { resolveFloat(v[0], ctx), resolveFloat(v[1], ctx), resolveFloat(v[2], ctx) };
    if (v.is_object()) {
        if (v.contains("rand")) {
            const auto& r = v["rand"];
            return { randRange(compOf(r["min"],0), compOf(r["max"],0), ctx),
                     randRange(compOf(r["min"],1), compOf(r["max"],1), ctx),
                     randRange(compOf(r["min"],2), compOf(r["max"],2), ctx) };
        }
        if (v.contains("lerp")) {
            const auto& l = v["lerp"];
            const std::string axis = l.value("axis", "col");
            const glm::vec3 from = resolveVec3(l["from"], ctx), to = resolveVec3(l["to"], ctx);
            return {
                lerpAxis(from.x, to.x, axis, ctx),
                lerpAxis(from.y, to.y, axis, ctx),
                lerpAxis(from.z, to.z, axis, ctx)
            };
        }
    }
    return {};
}

static std::string substituteToken(std::string s, const std::string& token, int index, int total) {
    const int width = static_cast<int>(std::to_string(total).size());

    // Named token (ex: `{TOKEN:NAME1,NAME2}` -> `NAME1`)
    const std::string labelPrefix = "{" + token + ":";
    size_t pos = s.find(labelPrefix);
    while (pos != std::string::npos) {
        const size_t closePos = s.find('}', pos);
        if (closePos == std::string::npos) break;

        std::vector<std::string> labels;
        std::istringstream ss(s.substr(pos + labelPrefix.size(), closePos - pos - labelPrefix.size()));
        std::string label;
        while (std::getline(ss, label, ',')) labels.push_back(label);

        std::string replacement;
        if (index < static_cast<int>(labels.size())) {
            replacement = labels[index];
        } else {
            replacement = std::format("{:0{}d}", index, width);
        }
        s.replace(pos, closePos - pos + 1, replacement);
        pos = s.find(labelPrefix, pos + replacement.size());
    }

    // Numerical field (ex: `{TOKEN}` -> `01`)
    const std::string plain = "{" + token + "}";
    while ((pos = s.find(plain)) != std::string::npos) {
        s.replace(pos, plain.size(), std::format("{:0{}d}", index, width));
    }

    return s;
}

std::string resolveTemplate(std::string tmpl, const ResolveCtx& ctx) {
    for (const auto& [token, indexCount] : ctx.tokens)
        tmpl = substituteToken(tmpl, token, indexCount.first, indexCount.second);
    return tmpl;
}
