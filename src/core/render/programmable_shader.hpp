#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/ecs/components/component.hpp"
#include "core/ecs/components/component_type.hpp"
#include "utils/glsl_dsl_file.hpp"

struct FrameContext;

class ProgrammableShader : private GlslDslFile {
public:
    using GlslDslFile::getError;

    ProgrammableShader();
    ~ProgrammableShader();
    ProgrammableShader(const ProgrammableShader&) = delete;
    ProgrammableShader& operator=(const ProgrammableShader&) = delete;
    ProgrammableShader(ProgrammableShader&&) = delete;
    ProgrammableShader& operator=(ProgrammableShader&&) = delete;

    bool parse(const std::filesystem::path& path);
    bool reload(const std::filesystem::path& path);

    std::vector<ecs::ComponentField>& getParams() { return params.getFields(); }
    const std::vector<ecs::ComponentField>& getParams() const { return params.getFields(); }
    ecs::ComponentField& getField(const std::string& id) { return params.getField(id); }
    ecs::Component& getComponent() { return params; }
    const std::string& getMainBody() const { return getStatements(); }
    const std::string& getOutBody() const { return getDeclarations(); }
    std::vector<float> packValues() const;

    int getSlot() const { return slot; }
    int getBaseOffset() const { return baseOffset; }

    static int slotForPath(const std::filesystem::path& path);
    static void packAll(const FrameContext& frame);
    static void generateDispatch();

private:
    static const size_t kShaderVersion = 1;
    static constexpr const char* KIND = "material";

    struct TypeInfo {
        FieldType fieldType;
        int components;
    };

    struct TypeSpec {
        const char* glsl;
        int components;
        bool isInt;
        bool isBool;
    };

    static const std::unordered_map<std::string, TypeInfo> typeTable;

    static std::optional<ecs::ComponentField> parseParam(const std::string& line, const std::filesystem::path& path, int lineNumber);
    static TypeSpec typeSpecFor(FieldType type);
    static size_t capacityFromCount(size_t count);

    void load(bool migrate);
    std::string mangledPrefix() const;
    std::string declareGlobal(const ecs::ComponentField& field) const;
    std::string assignParam(const ecs::ComponentField& field, int& offset) const;
    std::string generateGlobalDecls() const;
    std::string generateParamAssignments() const;

    ecs::ComponentType schema;
    ecs::Component params { schema };

    std::filesystem::path path;
    int slot = -1;
    int baseOffset = 0;
};
