#pragma once

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"

namespace ecs {

inline const ComponentType Material = ComponentType::builder("material")
    .description("Material marker.")
    .icon(ICON_FA_PALETTE)
    .group("material")
    .conflicts("transform")
    .build();

inline const ComponentType MaterialRef = ComponentType::builder("material_ref")
    .description("Material reference.")
    .icon(ICON_FA_PALETTE)
    .group("other")
    .field<ecs::Entity>("handle", ecs::Entity{}, EntityMeta{ .needs = {"material"} })
    .build();

inline const ComponentType Diffuse = ComponentType::builder("diffuse")
    .description("Diffuse BSDF.")
    .icon(ICON_FA_CIRCLE)
    .group("material")
    .needs("material")
    .conflicts("emissive", "metal", "glossy", "dielectric", "volume", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .build();

inline const ComponentType Emissive = ComponentType::builder("emissive")
    .description("Emissive light source BSDF.")
    .icon(ICON_FA_LIGHTBULB)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "metal", "glossy", "dielectric", "volume", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(1.0f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("emission_strength", 1.0f, NumericMeta{ .min = 0.0f, .step = 0.1f }, true)
    .build();

inline const ComponentType Metal = ComponentType::builder("metal")
    .description("GGX metallic BSDF.")
    .icon(ICON_FA_CIRCLE)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "glossy", "dielectric", "volume", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("roughness", 0.5f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType Glossy = ComponentType::builder("glossy")
    .description("GGX glossy dielectric BSDF.")
    .icon(ICON_FA_CIRCLE)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "metal", "dielectric", "volume", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("roughness", 0.5f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("ior", 1.5f, NumericMeta{ .min = 1.0f, .max = 3.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType Dielectric = ComponentType::builder("dielectric")
    .description("Dielectric refractive BSDF.")
    .icon(ICON_FA_CIRCLE)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "metal", "glossy", "volume", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(1.0f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("roughness", 0.0f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("ior", 1.5f, NumericMeta{ .min = 1.0f, .max = 3.0f, .step = 0.01f }, true)
    .field<float>("density", 0.0f, NumericMeta{ .min = 0.0f, .step = 0.01f }, true)
    .field<float>("transmission", 1.0f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("anisotropic", 0.0f, NumericMeta{ .min = -1.0f, .max = 1.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType Volume = ComponentType::builder("volume")
    .description("Homogeneous participating media BSDF.")
    .icon(ICON_FA_CIRCLE)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "metal", "glossy", "dielectric", "principled", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("density", 1.0f, NumericMeta{ .min = 0.0f, .step = 0.01f }, true)
    .field<float>("anisotropic", 0.0f, NumericMeta{ .min = -1.0f, .max = 1.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType Principled = ComponentType::builder("principled")
    .description("PBR principled BSDF.")
    .icon(ICON_FA_STAR)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "metal", "glossy", "dielectric", "volume", "programmable")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .field<float>("roughness", 0.5f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("metalness", 0.0f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("ior", 1.5f, NumericMeta{ .min = 1.0f, .max = 3.0f, .step = 0.01f }, true)
    .field<float>("transmission", 0.0f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("density", 0.0f, NumericMeta{ .min = 0.0f, .step = 0.01f }, true)
    .field<float>("anisotropic", 0.0f, NumericMeta{ .min = -1.0f, .max = 1.0f, .step = 0.01f }, true)
    .field<float>("alpha", 1.0f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType ProgrammableMaterial = ComponentType::builder("programmable")
    .description("Programmable custom BSDF.")
    .icon(ICON_FA_CODE)
    .group("material")
    .needs("material")
    .conflicts("diffuse", "emissive", "metal", "glossy", "dielectric", "volume", "principled")
    .field<glm::vec3>("albedo", glm::vec3(0.8f), NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f, .color = true }, true)
    .build();

}   // namespace ecs
