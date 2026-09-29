#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

struct CompositingPassEntry {
    std::string name = "Pass";
    std::filesystem::path path;
    std::unique_ptr<ShaderPlugin> plugin = std::make_unique<ShaderPlugin>();
};

class CompositingPasses {
public:
    std::vector<CompositingPassEntry> passes;
    int selected = -1;
};

inline const ComponentType Compositing = ComponentType::builder("compositing")
    .description("Compositing chain, an ordered list of programmable passes.")
    .icon(ICON_FA_LAYER_GROUP)
    .group("compositing")
    .conflicts("transform")
    .payload<CompositingPasses>("passes")
    .build();

}   // namespace ecs
