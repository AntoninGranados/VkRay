#pragma once

#include <fstream>

#include "nlohmann/json.hpp"

#include "core/parameters/parameters.hpp"

using json = nlohmann::ordered_json;

class ParameterSerializer {
public:
    static void saveDocumentation(std::filesystem::path path);
    static ParameterRegistry load(std::filesystem::path path);

private:
    static void serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth = 0);
    static void parseNode(const json& obj, ParameterRegistry& parameters, const std::string& path);

    template<typename T>
    static T readJsonVec(const json& arr);

    template<typename T>
    static Parameter& parseVecNode(const json& obj, ParameterRegistry& parameters, const std::string& path,
        const std::string& label, bool restart, T defMin, T defMax, float step);
};
