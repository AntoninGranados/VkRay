#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

struct ParameterOverride {
    std::string key;
    nlohmann::ordered_json value;
};

struct Job {
    std::filesystem::path scene;
    uint32_t seed = 0;
    std::vector<ParameterOverride> parameterOverrides;
};

class Offline {
public:
    static std::vector<Job> loadJobs(const std::filesystem::path& path);
    static int run(const std::vector<Job>& jobs);

private:
    static constexpr int kJobVersion = 1;

    Offline() = default;

    static void parseAovs(const nlohmann::ordered_json& names, std::vector<ParameterOverride>& overrides);
    static void initParameters(const std::vector<ParameterOverride>& overrides);
};
