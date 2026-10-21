#pragma once

#include <optional>
#include <string>
#include <vector>

#include "VkSmol/render/shader_file_system.hpp"

#include "utils/resources.hpp"

class ResourceShaderFileSystem final : public ShaderFileSystem {
public:
    std::optional<std::string> readFile(const std::string& path) override { return Resources::read(path); }
    std::vector<std::string> getSearchPaths() const override { return {"builtin:/shaders"}; }
};
