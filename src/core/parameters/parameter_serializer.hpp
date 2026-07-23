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
};