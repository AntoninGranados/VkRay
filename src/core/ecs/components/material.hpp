#pragma once

#include "core/scene/object/material.hpp"

namespace ecs {

struct MaterialRef {
    MaterialHandle handle = 0;

    void setHandle(MaterialHandle newHandle) {
        handle = newHandle;
    }
};

} // namespace ecs
