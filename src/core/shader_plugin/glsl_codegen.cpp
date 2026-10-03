#include "glsl_codegen.hpp"

#include <format>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "core/shader_plugin/shader_plugin.hpp"

#include "utils/resources.hpp"

namespace {
void appendShader(const std::filesystem::path& path, std::unordered_set<std::string>& included, std::string& code) {
    const std::string name = path.lexically_normal().generic_string();
    if (!included.insert(name).second) return;

    const std::optional<std::string> source = Resources::read(name);
    if (!source) throw std::runtime_error(std::format("Cannot open shader [{}]", name));

    if (!code.empty()) code += std::format("#line 1 \"{}\"\n", name);

    static const std::regex includePattern(R"re(^\s*#include\s+"([^"]+)")re");
    std::istringstream stream(*source);
    std::string line;
    for (int lineNumber = 1; std::getline(stream, line); lineNumber++) {
        std::smatch match;
        if (std::regex_search(line, match, includePattern)) {
            appendShader(path.parent_path() / match[1].str(), included, code);
            code += std::format("#line {} \"{}\"\n", lineNumber + 1, name);
            continue;
        }
        code += line + '\n';
        if (line.starts_with("#version"))
            code += std::format("#extension GL_GOOGLE_cpp_style_line_directive : require\n#line {} \"{}\"\n",
                                lineNumber + 1, name);
    }
}
} // namespace

std::string GlslCodegen::loadShader(const std::filesystem::path& path) {
    std::unordered_set<std::string> included;
    std::string code;
    appendShader(path, included, code);
    return code;
}

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
