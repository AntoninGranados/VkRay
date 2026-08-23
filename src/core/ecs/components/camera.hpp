#pragma once

#include <filesystem>

#include "FontAwesome/IconsFontAwesome7.h"

#include <glm/glm.hpp>

#include "core/ecs/components/component_type.hpp"

namespace ecs {

inline const ComponentType Camera = ComponentType::builder("camera")
    .description("Perspective camera.")
    .icon(ICON_FA_VIDEO)
    .group("camera")
    .needs("transform")
    .conflicts("sphere", "plane", "box", "quad", "mesh_ref")
    .field<float>("fov", 80.0f, NumericMeta{ .min = 1.0f, .max = 160.0f, .step = 0.1f }, true)
    .field<float>("shutter_speed", 0.0f, NumericMeta{ .min = 0.0f, .step = 0.001f, .presets = {
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

struct ThinLensSync {
    float lastFov = 0.0f;
    float lastFocalLength = 0.0f;
    bool initialized = false;
};

inline const ComponentType ThinLens = ComponentType::builder("thin_lens")
    .description("Depth-of-field via thin lens approximation.")
    .icon(ICON_FA_CIRCLE_DOT)
    .group("camera")
    .needs("camera")
    .field<float>("focal_length", 21.45f, NumericMeta{ .min = 1.0f, .max = 300.0f, .step = 0.5f }, true)
    .field<float>("focal_distance", 10.0f, NumericMeta{ .min = 0.1f, .step = 0.01f }, true)
    .payload<ThinLensSync>("sync")
    .field<float>("f_stop", 0.0f, NumericMeta{ .min = 0.0f, .max = 64.0f, .step = 0.1f, .presets = {
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
    .field<bool>("show_focus_plane", false)
    .build();

inline const ComponentType TiltShiftLens = ComponentType::builder("tilt_shift_lens")
    .description("Tilted focal plane and lens shift (Scheimpflug principle).")
    .icon(ICON_FA_EXPAND)
    .group("camera")
    .needs("thin_lens")
    .field<glm::vec3>("plane_position", glm::vec3(0.0f, 0.0f, 0.0f), NumericMeta{ .step = 0.1f }, true)
    .field<glm::vec3>("plane_rotation", glm::vec3(0.0f, 0.0f, 0.0f), NumericMeta{ .step = 1.0f }, true)
    .build();

inline const ComponentType GeometricAperture = ComponentType::builder("geometric_aperture")
    .description("Polygon aperture blade shape.")
    .icon(ICON_FA_STAR)
    .group("camera")
    .needs("thin_lens")
    .conflicts("image_aperture")
    .field<int>("blades", 6, NumericMeta{ .min = 3, .max = 12, .step = 1 })
    .field<float>("rotation", 0.0f, NumericMeta{ .min = 0.0f, .max = 360.0f, .step = 1.0f })
    .build();

inline const ComponentType ImageAperture = ComponentType::builder("image_aperture")
    .description("Custom image mask as aperture shape.")
    .icon(ICON_FA_IMAGE)
    .group("camera")
    .needs("thin_lens")
    .conflicts("geometric_aperture")
    .field<std::filesystem::path>("path", {}, PathMeta{ .extensions = {{ .ext = "pgm,png,jpg,jpeg,hdr", .name = "Image" }}, .presets = {
        {"ring",    "assets/apertures/ring.pgm"},
        {"star",    "assets/apertures/star.pgm"},
        {"heart",   "assets/apertures/heart.pgm"},
        {"cat eye", "assets/apertures/cat_eye.pgm"}
    } })
    .build();

}   // namespace ecs
