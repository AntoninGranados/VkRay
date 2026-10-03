#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

class ShaderPlugin;

namespace GlslCodegen {
std::string loadShader(const std::filesystem::path& path);

std::string declareGlobals(const ShaderPlugin& plugin);
std::string assignParams(const ShaderPlugin& plugin, const std::function<std::string(int)>& valueAt);

int slotFor(std::unordered_map<std::filesystem::path, int>& slots, const std::filesystem::path& path);

ShaderPlugin* findActivePlugin(const std::string& type);
void forEachActivePlugin(const std::string& type,
                         const std::function<void(ShaderPlugin&, const std::string& funcName)>& fn);
} // namespace GlslCodegen
