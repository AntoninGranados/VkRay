#pragma once

#include <string>

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"

namespace ecs {

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
    .field<glm::vec3>("position", glm::vec3(0.0f), NumericMeta{ .step = 0.1f }, true)
    .field<glm::vec3>("rotation", glm::vec3(0.0f), NumericMeta{ .step = 0.1f }, true)
    .field<glm::vec3>("scale", glm::vec3(1.0f), NumericMeta{ .step = 0.1f }, true)
    .build();

}   // namespace ecs
