#pragma once

// ECS-side definitions of the scene primitives; see core/scene/gpu_structs.hpp for their GPU-side mirror.

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/scene/asset/mesh.hpp"

namespace ecs {

struct MeshAreaCache {
    glm::vec3 scale{0.0f};
    uint64_t changeTick = 0;
    float area = 0.0f;
    bool valid = false;
};

// clang-format off

inline const ComponentType Sphere = ComponentType::builder("sphere")
    .description("Sphere primitive.")
    .icon(ICON_FA_CIRCLE)
    .group("object")
    .needs("transform")
    .slot("shape")
    .build();

inline const ComponentType Plane = ComponentType::builder("plane")
    .description("Infinite plane primitive.")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .slot("shape")
    .build();

inline const ComponentType Box = ComponentType::builder("box")
    .description("Box primitive.")
    .icon(ICON_FA_BOX)
    .group("object")
    .needs("transform")
    .slot("shape")
    .build();

inline const ComponentType Quad = ComponentType::builder("quad")
    .description("Single face quad primitive.")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .slot("shape")
    .build();

inline const ComponentType Mesh = ComponentType::builder("mesh")
    .description("Mesh geometry asset loaded from file.")
    .icon(ICON_FA_CUBE)
    .group("asset")
    .kind()
    .permanent()
    .field<std::filesystem::path>("path", {})
    .field<bool>("smooth", false)
    .payload<MeshAsset>("geometry")
    .build();

inline const ComponentType MeshSimplify = ComponentType::builder("mesh_simplify")
    .description("Simplifies the mesh asset to a target ratio.")
    .icon(ICON_FA_CUBE)
    .group("asset")
    .needs("mesh")
    .field<float>("ratio", 1.0f, NumericMeta{ .min = 0.05f, .max = 1.0f, .step = 0.01f })
    .payload<MeshAsset>("original")
    .build();

inline const ComponentType MeshRef = ComponentType::builder("mesh_ref")
    .description("Reference to a mesh asset.")
    .icon(ICON_FA_CUBE)
    .group("object")
    .needs("transform")
    .slot("shape")
    .field<ecs::Entity>("handle", ecs::Entity{}, EntityMeta{ .needs = {"mesh"} })
    .payload<MeshAreaCache>("area_cache")
    .build();

// clang-format on

} // namespace ecs
