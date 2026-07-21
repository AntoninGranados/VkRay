#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

enum class JobStatus { Pending, Running, Done, Failed };

using ParameterValue = std::variant<
    bool, int, float,
    std::string, std::filesystem::path,
    glm::ivec2, glm::ivec3, glm::ivec4,
    glm::vec2, glm::vec3, glm::vec4
>;

struct ParameterOverride {
    std::string key;
    ParameterValue value;
};

struct Job {
    std::filesystem::path scene;
    uint32_t seed;
    std::vector<ParameterOverride> parameterOverrides;

    JobStatus status = JobStatus::Pending;
    float progress = 0.0f;
};
