#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/fields/field.hpp"

struct GlslTypeInfo {
    std::string name;
    FieldType fieldType;
    int components;
    bool isInt;
    bool isBool;
};

class ShaderScript {
public:
    struct ParseResult {
        bool ok = false;
        std::string error;
        std::vector<Field> fields;
        std::string body;
    };

    static ParseResult parse(const std::filesystem::path& path, const std::string& type, int version);

    static const GlslTypeInfo* findGlslType(const std::string& name);
    static const GlslTypeInfo* findGlslType(FieldType fieldType);

private:
    static const std::vector<GlslTypeInfo> glslTypes;

    static std::optional<Field> parseParam(const std::string& group, const std::string& line, const std::filesystem::path& path, int lineNumber);
};
