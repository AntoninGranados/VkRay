#include "glsl_codegen.hpp"

#include <format>

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
            if (p.isInt) args += std::format("int({})", value);
            else if (p.isBool) args += std::format("bool({})", value);
            else args += value;
        }
        assignments += p.components == 1
            ? std::format("{} = {};\n", p.mangled, args)
            : std::format("{} = {}({});\n", p.mangled, p.glslType, args);
        offset += p.components;
    }
    return assignments;
}
