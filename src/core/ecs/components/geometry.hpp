#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"

namespace ecs {

inline const ComponentType Sphere = ComponentType::builder("sphere")
    .description("Sphere primitive.")
    .icon(ICON_FA_CIRCLE)
    .group("object")
    .needs("transform")
    .conflicts("plane", "box", "mesh", "camera")
    .field<float>("radius", 1.0f, { .min = 0.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType Plane = ComponentType::builder("plane")
    .description("Infinite plane primitive.")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "box", "mesh", "quad", "camera")
    .build();

inline const ComponentType Box = ComponentType::builder("box")
    .description("Box primitive.")
    .icon(ICON_FA_BOX)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "mesh", "quad", "camera")
    .build();

inline const ComponentType Quad = ComponentType::builder("quad")
    .description("Single face quad primitive.")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "mesh", "camera")
    .build();

inline const ComponentType MeshRef = ComponentType::builder("mesh")
    .description("Loaded mesh asset.")
    .icon(ICON_FA_CUBE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "quad", "camera")
    .field<int>("handle")
    .build();

}   // namespace ecs
