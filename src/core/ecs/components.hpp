#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include <filesystem>
#include <glm/glm.hpp>

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

inline const ComponentType Camera = ComponentType::builder("camera")
    .description("Perspective camera.")
    .icon(ICON_FA_VIDEO)
    .group("object")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "quad", "mesh")
    .field<float>("fov", 80.0f, { .min = 1.0f, .max = 160.0f, .step = 0.1f }, true)
    .field<float>("shutter_speed", 0.0f, { .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .privateField<bool>("_is_preview")
    .build();

inline const ComponentType ThinLensCamera = ComponentType::builder("thin_lens_camera")
    .description("Depth-of-field via thin lens approximation.")
    .icon(ICON_FA_CIRCLE_DOT)
    .group("object")
    .needs("camera")
    .field<float>("aperture", 0.0f, { .min = 0.0f, .max = 10.0f, .step = 0.01f }, true)
    .field<float>("focus_depth", 10.0f, { .min = 0.0f, .step = 0.01f }, true)
    .build();

inline const ComponentType GeometricAperture = ComponentType::builder("geometric_aperture")
    .description("Polygon aperture blade shape.")
    .icon(ICON_FA_STAR)
    .group("object")
    .needs("thin_lens_camera")
    .conflicts("image_aperture")
    .field<int>("blades", 6, { .min = 3, .max = 12, .step = 1 })
    .field<float>("rotation", 0.0f, { .min = 0.0f, .max = 360.0f, .step = 1.0f })
    .build();

inline const ComponentType ImageAperture = ComponentType::builder("image_aperture")
    .description("Custom image mask as aperture shape.")
    .icon(ICON_FA_IMAGE)
    .group("object")
    .needs("thin_lens_camera")
    .conflicts("geometric_aperture")
    .field<std::filesystem::path>("path", {}, { .pathExtensions = {{ .ext = "pgm,png,jpg,jpeg,hdr", .name = "Image" }} })
    .build();

inline const ComponentType Collider = ComponentType::builder("collider")
    .description("Physics collider shape.")
    .icon(ICON_FA_SQUARE)
    .group("physics")
    .field<float>("restitution", 0.4f, { .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .field<float>("friction", 0.4f, { .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .build();

inline const ComponentType RigidBody = ComponentType::builder("rigid_body")
    .description("Physics rigid body.")
    .icon(ICON_FA_CUBES_STACKED)
    .group("physics")
    .field<bool>("use_gravity", true)
    .field<float>("density", 50.0f, { .min = 0.1f, .max = 10000.0f, .step = 1.0f })
    .privateField<glm::vec3>("_linear_velocity")
    .privateField<glm::vec3>("_angular_velocity")
    .build();

inline const ComponentType Name = ComponentType::builder("name")
    .description("Display name.")
    .icon(ICON_FA_TAG)
    .group("other")
    .field<std::string>("value")
    .build();

inline const ComponentType Transform = ComponentType::builder("transform")
    .description("World-space transform.")
    .icon(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT)
    .group("movement")
    .field<glm::vec3>("position", glm::vec3(0.0f), { .step = 0.1f }, true)
    .field<glm::vec3>("rotation", glm::vec3(0.0f), { .step = 0.1f }, true)
    .field<glm::vec3>("scale", glm::vec3(1.0f), { .step = 0.1f }, true)
    .build();

inline const ComponentType MaterialRef = ComponentType::builder("material")
    .description("Material reference.")
    .icon(ICON_FA_PALETTE)
    .group("other")
    .field<int>("handle")
    .build();

}   // namespace ecs
