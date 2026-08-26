#pragma once

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"

namespace ecs {

enum class DragMode {
    None,
    Look,
    Orbit,
    Pan,
    Dolly
};

struct CameraNavigationState {
    DragMode dragMode = DragMode::None;
    bool locked = false;

    float orbitDistance = 10.0f;

    bool firstMouse = true;
    float lastX = 0.0f, lastY = 0.0f;

    glm::vec3 anchor = glm::vec3(0.0f);
};

inline const ComponentType CameraNavigation = ComponentType::builder("camera_navigation")
    .description("Live interactive navigation state for the active camera.")
    .icon(ICON_FA_VIDEO)
    .group("internal")
    .needs("camera")
    .payload<CameraNavigationState>("state")
    .build();

}   // namespace ecs
