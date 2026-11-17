#pragma once

#include <filesystem>
#include <format>
#include <string>

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/shader_plugin/shader_plugin.hpp"

namespace ecs {

inline ComponentType::Builder builtinMaterial(const std::string& id, const std::string& description,
                                              const std::string& icon) {
    ComponentType::Builder builder = ComponentType::builder(id);
    builder.description(description)
        .icon(icon)
        .group("material")
        .needs("material")
        .slot("bsdf")
        .script(std::format("builtin:/shaders/core/materials/{}.slang", id))
        .payload<ShaderPlugin>("plugin");
    return builder;
}

// clang-format off

inline const ComponentType& Material = ComponentType::builder("material")
    .description("Material marker.")
    .icon(ICON_FA_PALETTE)
    .group("material")
    .kind()
    .permanent()
    .build();

inline const ComponentType& MaterialRef = ComponentType::builder("material_ref")
    .description("Material reference.")
    .icon(ICON_FA_PALETTE)
    .group("material")
    .needs("transform")
    .field<ecs::Entity>("handle", ecs::Entity{}, EntityMeta{ .needs = {"material"} })
    .build();

inline const ComponentType& Diffuse = builtinMaterial("diffuse", "Diffuse BSDF.", ICON_FA_CIRCLE).build();
inline const ComponentType& Emissive = builtinMaterial("emissive", "Emissive light source BSDF.", ICON_FA_LIGHTBULB).build();
inline const ComponentType& Metal = builtinMaterial("metal", "GGX metallic BSDF.", ICON_FA_CIRCLE).build();
inline const ComponentType& Glossy = builtinMaterial("glossy", "GGX glossy dielectric BSDF.", ICON_FA_CIRCLE).build();
inline const ComponentType& Dielectric = builtinMaterial("dielectric", "Dielectric refractive BSDF.", ICON_FA_CIRCLE).build();
inline const ComponentType& Volume = builtinMaterial("volume", "Homogeneous participating media BSDF.", ICON_FA_CIRCLE).build();
inline const ComponentType& Principled = builtinMaterial("principled", "PBR principled BSDF.", ICON_FA_STAR).build();

inline const ComponentType& MaterialPlugin = ComponentType::builder("programmable")
    .description("Programmable material, defined by a Slang script implementing IBsdf or IMaterial.")
    .icon(ICON_FA_CODE)
    .group("material")
    .needs("material")
    .slot("bsdf")
    .field<std::filesystem::path>("path", {}, PathMeta{ .extensions = {{ .ext = "slang", .name = "Material Shader" }} })
    .payload<ShaderPlugin>("plugin")
    .build();

// clang-format on

} // namespace ecs
