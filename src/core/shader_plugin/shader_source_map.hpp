#pragma once

#include <filesystem>
#include <string>

namespace ShaderSourceMap {
std::string marker(const std::filesystem::path& path, int line);
std::string remapError(const std::string& message);
} // namespace ShaderSourceMap
