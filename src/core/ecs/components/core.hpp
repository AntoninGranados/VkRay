#pragma once

#include <string>

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"

namespace ecs {

// clang-format off

inline const ComponentType Name = ComponentType::builder("name")
    .description("Display name.")
    .icon(ICON_FA_TAG)
    .group("other")
    .permanent()
    .field<std::string>("value")
    .build();

inline const ComponentType Locked = ComponentType::builder("locked")
    .description("Prevents the entity from being deleted in the editor.")
    .icon(ICON_FA_LOCK)
    .group("internal")
    .build();

inline const ComponentType Transform = ComponentType::builder("transform")
    .description("World-space transform.")
    .icon(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT)
    .group("movement")
    .kind()
    .permanent()
    .field<glm::vec3>("position", glm::vec3(0.0f), NumericMeta{ .step = 0.1f }, true)
    .field<glm::vec3>("rotation", glm::vec3(0.0f), NumericMeta{ .step = 0.1f }, true)
    .field<glm::vec3>("scale", glm::vec3(1.0f), NumericMeta{ .min = 1e-8f, .step = 0.1f, .linkable = true }, true)
    .build();

// clang-format on

} // namespace ecs
