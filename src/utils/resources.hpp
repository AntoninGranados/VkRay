#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Resources {
inline constexpr std::string_view kBuiltinPrefix = "builtin:/";

bool isBuiltin(const std::filesystem::path& path);
void add(const std::filesystem::path& path, std::string content);
std::optional<std::string_view> find(const std::filesystem::path& path);
std::optional<std::string> read(const std::filesystem::path& path);
} // namespace Resources
