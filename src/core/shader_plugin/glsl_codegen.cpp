#include "glsl_codegen.hpp"

#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "core/shader_plugin/shader_plugin.hpp"

std::string GlslCodegen::declareGlobals(const ShaderPlugin& plugin) {
    std::string decls;
    for (const ShaderPlugin::PluginParam& p : plugin.getParameters())
        decls += std::format("{} {};\n", p.glslType, p.mangled);
    return decls;
}

std::string GlslCodegen::assignParams(const ShaderPlugin& plugin, const std::function<std::string(int)>& valueAt) {
    std::string assignments;
    int offset = 0;
    for (const ShaderPlugin::PluginParam& p : plugin.getParameters()) {
        std::string args;
        for (int i = 0; i < p.components; i++) {
            if (i > 0) args += ", ";
            const std::string value = valueAt(offset + i);
            if (p.isInt)
                args += std::format("int({})", value);
            else if (p.isBool)
                args += std::format("bool({})", value);
            else
                args += value;
        }
        assignments += p.components == 1 ? std::format("{} = {};\n", p.mangled, args)
                                         : std::format("{} = {}({});\n", p.mangled, p.glslType, args);
        offset += p.components;
    }
    return assignments;
}

int GlslCodegen::slotFor(std::unordered_map<std::filesystem::path, int>& slots, const std::filesystem::path& path) {
    const auto [it, inserted] = slots.try_emplace(path, static_cast<int>(slots.size()));
    return it->second;
}

void GlslCodegen::writeGeneratedFileIfChanged(const std::filesystem::path& outputPath, const std::string& content) {
    std::error_code ec;
    std::filesystem::create_directories(outputPath.parent_path(), ec);

    std::ifstream existing(outputPath);
    std::stringstream existingBuffer;
    existingBuffer << existing.rdbuf();
    if (existingBuffer.str() == content) return;

    std::ofstream(outputPath) << content;
}

namespace {
bool isActive(const ShaderPlugin& plugin, const std::string& type) {
    return plugin.getType() == type && plugin.getError().empty() && !plugin.getBody().empty();
}
} // namespace

ShaderPlugin* GlslCodegen::findActivePlugin(const std::string& type) {
    for (ShaderPlugin* plugin : ShaderPlugin::registry())
        if (isActive(*plugin, type)) return plugin;
    return nullptr;
}

void GlslCodegen::forEachActivePlugin(const std::string& type,
                                      const std::function<void(ShaderPlugin&, const std::string&)>& fn) {
    std::unordered_set<int> emittedSlots;
    for (ShaderPlugin* plugin : ShaderPlugin::registry()) {
        if (!isActive(*plugin, type)) continue;
        if (!emittedSlots.insert(plugin->getSlot()).second) continue;
        fn(*plugin, plugin->getPrefix() + "programmable");
    }
}
