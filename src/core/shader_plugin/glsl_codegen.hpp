#pragma once

#include <functional>
#include <string>

class ShaderPlugin;

namespace GlslCodegen {
    std::string declareGlobals(const ShaderPlugin& plugin);
    std::string assignParams(const ShaderPlugin& plugin, const std::function<std::string(int)>& valueAt);
}
