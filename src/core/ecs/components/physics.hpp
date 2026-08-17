#pragma once

#include <glm/glm.hpp>

#include "FontAwesome/IconsFontAwesome7.h"

#include "core/ecs/components/component_type.hpp"
#include "core/ecs/systems/physics/physics_solver.hpp"

namespace ecs {

inline const ComponentType Collider = ComponentType::builder("collider")
    .description("Physics collider shape.")
    .icon(ICON_FA_SQUARE)
    .group("physics")
    .field<float>("restitution", 0.4f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .field<float>("friction", 0.4f, NumericMeta{ .min = 0.0f, .max = 1.0f, .step = 0.01f })
    .payload<physics_detail::ColliderState>("prev_transform")
    .build();

inline const ComponentType RigidBody = ComponentType::builder("rigid_body")
    .description("Physics rigid body.")
    .icon(ICON_FA_CUBES_STACKED)
    .group("physics")
    .field<bool>("use_gravity", true)
    .field<float>("density", 50.0f, NumericMeta{ .min = 0.1f, .max = 10000.0f, .step = 1.0f })
    .payload<physics_detail::BodyState>("state")
    .build();

}   // namespace ecs
