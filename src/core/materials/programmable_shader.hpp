#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/field.hpp"

class ProgrammableShader {
public:
    bool parse(const std::filesystem::path& path);

    std::vector<Field>& getParams() { return params; }
    const std::vector<Field>& getParams() const { return params; }
    const std::string& getError() const { return error; }

private:
    struct TypeInfo {
        FieldType fieldType;
        int components;
    };

    static const std::unordered_map<std::string, TypeInfo> typeTable;

    static std::optional<Field> parseParam(const std::string& statement);

    std::vector<Field> params;
    std::string error;
};
