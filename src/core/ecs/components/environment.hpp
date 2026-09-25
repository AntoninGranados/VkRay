#pragma once

#include <filesystem>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

inline const ComponentType Environment = ComponentType::builder("environment")
    .description("Environment marker.")
    .icon(ICON_FA_CLOUD_SUN)
    .group("environment")
    .conflicts("transform")
    .build();

inline const ComponentType SkyPlugin = ComponentType::builder("programmable_sky")
    .description("Programmable custom sky, defined by a GLSL shader-definition file.")
    .icon(ICON_FA_CODE)
    .group("environment")
    .needs("environment")
    .field<std::filesystem::path>("path", {}, PathMeta{ .extensions = {{ .ext = "glsl", .name = "Sky Shader" }} })
    .payload<ShaderPlugin>("plugin")
    .build();

}   // namespace ecs
