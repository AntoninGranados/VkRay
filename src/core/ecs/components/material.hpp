#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

inline constexpr const char* kDiffusePath = "builtin:/shaders/core/materials/diffuse.slang";
inline constexpr const char* kEmissivePath = "builtin:/shaders/core/materials/emissive.slang";

// clang-format off

inline const ComponentType Material = ComponentType::builder("material")
    .description("Material marker.")
    .icon(ICON_FA_PALETTE)
    .group("material")
    .kind()
    .permanent()
    .build();

inline const ComponentType MaterialRef = ComponentType::builder("material_ref")
    .description("Material reference.")
    .icon(ICON_FA_PALETTE)
    .group("material")
    .needs("transform")
    .field<ecs::Entity>("handle", ecs::Entity{}, EntityMeta{ .needs = {"material"} })
    .build();

inline const ComponentType MaterialPlugin = ComponentType::builder("programmable")
    .description("Programmable material, defined by a Slang script implementing IBsdf or IMaterial.")
    .icon(ICON_FA_CODE)
    .group("material")
    .needs("material")
    .slot("bsdf")
    .field<std::filesystem::path>("path", {}, PathMeta{ .extensions = {{ .ext = "slang", .name = "Material Shader" }}, .presets = {
        {"principled", "builtin:/shaders/core/materials/principled.slang"},
        {"diffuse",    kDiffusePath},
        {"emissive",   kEmissivePath},
        {"metal",      "builtin:/shaders/core/materials/metal.slang"},
        {"glossy",     "builtin:/shaders/core/materials/glossy.slang"},
        {"dielectric", "builtin:/shaders/core/materials/dielectric.slang"},
        {"volume",     "builtin:/shaders/core/materials/volume.slang"}
    } })
    .payload<ShaderPlugin>("plugin")
    .build();

// clang-format on

} // namespace ecs
