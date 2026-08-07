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
    .field<float>("shutter_speed", 0.0f, { .min = 0.0f, .step = 0.001f, .presets = {
        {"Off",    0.0f},
        {"1/8000", 1.0f/8000.0f},
        {"1/4000", 1.0f/4000.0f},
        {"1/2000", 1.0f/2000.0f},
        {"1/1000", 1.0f/1000.0f},
        {"1/500",  1.0f/500.0f},
        {"1/250",  1.0f/250.0f},
        {"1/125",  1.0f/125.0f},
        {"1/60",   1.0f/60.0f},
        {"1/30",   1.0f/30.0f},
        {"1/15",   1.0f/15.0f},
        {"1/8",    1.0f/8.0f},
        {"1/4",    1.0f/4.0f},
        {"1/2",    1.0f/2.0f},
        {"1s",     1.0f}
    } })
    .build();

inline const ComponentType ThinLens = ComponentType::builder("thin_lens")
    .description("Depth-of-field via thin lens approximation.")
    .icon(ICON_FA_CIRCLE_DOT)
    .group("object")
    .needs("camera")
    .field<float>("focal_length", 1.0f, { .min = 0.05f, .max = 10.0f, .step = 0.01f }, true)
    .field<float>("focal_distance", 10.0f, { .min = 0.1f, .step = 0.01f }, true)
    .field<float>("f_stop", 0.0f, { .min = 0.0f, .max = 64.0f, .step = 0.1f, .presets = {
        {"Off",   0.0f},
        {"f/1",   1.0f},
        {"f/1.4", 1.4f},
        {"f/2",   2.0f},
        {"f/2.8", 2.8f},
        {"f/4",   4.0f},
        {"f/5.6", 5.6f},
        {"f/8",   8.0f},
        {"f/11",  11.0f},
        {"f/16",  16.0f},
        {"f/22",  22.0f},
        {"f/32",  32.0f}
    } }, true)
    .build();

inline const ComponentType GeometricAperture = ComponentType::builder("geometric_aperture")
    .description("Polygon aperture blade shape.")
    .icon(ICON_FA_STAR)
    .group("object")
    .needs("thin_lens")
    .conflicts("image_aperture")
    .field<int>("blades", 6, { .min = 3, .max = 12, .step = 1 })
    .field<float>("rotation", 0.0f, { .min = 0.0f, .max = 360.0f, .step = 1.0f })
    .build();

inline const ComponentType ImageAperture = ComponentType::builder("image_aperture")
    .description("Custom image mask as aperture shape.")
    .icon(ICON_FA_IMAGE)
    .group("object")
    .needs("thin_lens")
    .conflicts("geometric_aperture")
    .field<std::filesystem::path>("path", {}, { .pathExtensions = {{ .ext = "pgm,png,jpg,jpeg,hdr", .name = "Image" }}, .presets = {
        {"ring",    "assets/apertures/ring.pgm"},
        {"star",    "assets/apertures/star.pgm"},
        {"heart",   "assets/apertures/heart.pgm"},
        {"cat eye", "assets/apertures/cat_eye.pgm"}
    } })
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
