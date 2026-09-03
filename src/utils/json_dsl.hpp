#pragma once

#include <random>
#include <string>
#include <unordered_map>
#include <utility>

#include <glm/glm.hpp>

#include "nlohmann/json.hpp"

struct ResolveCtx {
    std::mt19937& rng;
    std::unordered_map<std::string, std::pair<int, int>> tokens;
};

bool expectArray(const nlohmann::ordered_json& v, size_t n, const std::string& fieldId);

float     resolveFloat(const nlohmann::ordered_json& v, const ResolveCtx& ctx);
glm::vec2 resolveVec2(const nlohmann::ordered_json& v, const ResolveCtx& ctx);
glm::vec3 resolveVec3(const nlohmann::ordered_json& v, const ResolveCtx& ctx);

// Resolve a format string of the shape `out_{row}_{col}_{spp:clean,raw}.png` based on the context `ctx`
// - `{n}`, `{row}`, `{col}` and `{spp}` will be resolved as numerical values
// - `{TOKEN:NAME1,NAME2,...}` will be resolved as `NAME1` or `NAME2` (or ...) based on the index
std::string resolveTemplate(std::string tmpl, const ResolveCtx& ctx);

std::string trimmed(const std::string& s);

std::string prettifyJson(const nlohmann::ordered_json& j, int indent = 0);
