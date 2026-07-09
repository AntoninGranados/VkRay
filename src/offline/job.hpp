#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

enum class JobStatus { Pending, Running, Done, Failed };

using ParamValue = std::variant<bool, int, float, std::string>;

struct ParamOverride {
    std::string key;
    ParamValue  value;
};

struct Job {
    std::filesystem::path      scene;
    std::filesystem::path      output;
    uint32_t                   samples;
    uint32_t                   seed;
    uint32_t                   width;
    uint32_t                   height;
    std::vector<ParamOverride> parameterOverrides;

    JobStatus status   = JobStatus::Pending;
    float     progress = 0.0f;
};
