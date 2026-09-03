#include "utils/json_dsl.hpp"

#include <algorithm>
#include <format>
#include <sstream>
#include <vector>

#include "utils/log.hpp"

using json = nlohmann::ordered_json;

bool expectArray(const json& v, size_t n, const std::string& fieldId) {
    if (v.is_array() && v.size() >= n) return true;
    Log::error("JsonDsl", std::format("Field '{}': expected array of {}, got: {}", fieldId, n, v.dump()));
    return false;
}

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

std::string trimmed(const std::string& s) {
    const auto pos = s.find('\0');
    return pos != std::string::npos ? s.substr(0, pos) : s;
}

static std::string inlineJson(const json& j) {
    if (j.is_object()) {
        std::string s = "{ ";
        bool first = true;
        for (const auto& [k, v] : j.items()) {
            if (!first) s += ", ";
            s += '"' + k + "\": " + inlineJson(v);
            first = false;
        }
        return s + " }";
    }
    if (j.is_array()) {
        std::string s = "[";
        for (size_t i = 0; i < j.size(); i++) {
            if (i > 0) s += ", ";
            s += inlineJson(j[i]);
        }
        return s + "]";
    }
    return j.dump();
}

static bool isInlineVector(const json& v) { return v.is_array() && std::all_of(v.begin(), v.end(), [](const json& e){ return e.is_number(); }); }
static bool isInlineExpression(const json& v) { return v.is_object() && v.size() == 1 && (v.contains("rand") || v.contains("lerp")); }
static bool isInlineKeyframe(const json& v) {
    if (!v.is_object()) return false;
    const size_t n = v.size();
    return (n == 2 || n == 3) && v.contains("frame") && v.contains("value") && (n == 2 || v.contains("ease"));
}

std::string prettifyJson(const json& j, int indent) {
    if (isInlineVector(j) || isInlineExpression(j) || isInlineKeyframe(j)) return inlineJson(j);

    if (j.is_object()) {
        std::string inlined = inlineJson(j);
        if (inlined.size() <= 60) return inlined;
    }

    const std::string pad(indent * 4, ' ');
    const std::string inner((indent + 1) * 4, ' ');

    if (j.is_array()) {
        std::string s = "[\n";
        for (size_t i = 0; i < j.size(); i++) {
            s += inner + prettifyJson(j[i], indent + 1);
            if (i + 1 < j.size()) s += ',';
            s += '\n';
        }
        return s + pad + "]";
    }
    if (j.is_object()) {
        std::string s = "{\n";
        size_t i = 0;
        for (const auto& [k, v] : j.items()) {
            s += inner + '"' + k + "\": " + prettifyJson(v, indent + 1);
            if (++i < j.size()) s += ',';
            s += '\n';
        }
        return s + pad + "}";
    }
    return j.dump();
}

