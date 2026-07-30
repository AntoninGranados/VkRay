#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include <glm/glm.hpp>

#include "core/ecs/components/component_type.hpp"

namespace ecs {

inline const ComponentType Sphere = ComponentType::builder("sphere")
    .icon(ICON_FA_CIRCLE)
    .group("object")
    .needs("transform")
    .conflicts("plane", "box", "mesh", "camera")
    .field<float>("radius", 1.0f, { .min = 0.0f, .step = 0.01f, .animatable = true })
    .build();

inline const ComponentType Plane = ComponentType::builder("plane")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "box", "mesh", "quad", "camera")
    .build();

inline const ComponentType Box = ComponentType::builder("box")
    .icon(ICON_FA_BOX)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "mesh", "quad", "camera")
    .build();

inline const ComponentType Quad = ComponentType::builder("quad")
    .icon(ICON_FA_SQUARE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "mesh", "camera")
    .build();

inline const ComponentType MeshRef = ComponentType::builder("mesh")
    .icon(ICON_FA_CUBE)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "quad", "camera")
    .field<int>("handle")
    .build();

inline const ComponentType Camera = ComponentType::builder("camera")
    .icon(ICON_FA_VIDEO)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "quad", "mesh")
    .field<float>("fov", 80.0f, { .min = 1.0f, .max = 160.0f, .step = 0.1f, .animatable = true })
    .field<float>("aperture", 0.0f, { .min = 0.0f, .max = 10.0f, .step = 0.01f, .animatable = true })
    .field<float>("focus_depth", 10.0f, { .min = 0.0f, .step = 0.01f, .animatable = true })
    .privateField<bool>("is_preview")
    .build();

inline const ComponentType Collider = ComponentType::builder("collider")
    .icon(ICON_FA_SQUARE)
    .group("physics")
    .field<float>("restitution", 0.4f, { .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .field<float>("friction", 0.4f, { .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .build();

inline const ComponentType RigidBody = ComponentType::builder("rigid_body")
    .icon(ICON_FA_CUBES_STACKED)
    .group("physics")
    .field<bool>("use_gravity", true)
    .field<float>("density", 50.0f, { .min = 0.1f, .max = 10000.0f, .step = 1.0f })
    .privateField<glm::vec3>("linear_velocity")
    .privateField<glm::vec3>("angular_velocity")
    .build();

inline const ComponentType Name = ComponentType::builder("name")
    .icon(ICON_FA_TAG)
    .group("other")
    .field<std::string>("value")
    .build();

inline const ComponentType Transform = ComponentType::builder("transform")
    .icon(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT)
    .group("movement")
    .field<glm::vec3>("position", glm::vec3(0.0f), { .animatable = true })
    .field<glm::vec3>("rotation", glm::vec3(0.0f), { .step = 0.1f, .animatable = true })
    .field<glm::vec3>("scale", glm::vec3(1.0f), { .animatable = true })
    .build();

inline const ComponentType MaterialRef = ComponentType::builder("material")
    .icon(ICON_FA_PALETTE)
    .group("other")
    .field<int>("handle")
    .build();

}   // namespace ecs
