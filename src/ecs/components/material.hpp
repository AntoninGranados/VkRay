#pragma once

#include "../../scene/object/material.hpp"

namespace ecs {

struct MaterialRef {
    MaterialHandle handle = -1;

    void setHandle(MaterialHandle newHandle) {
        handle = newHandle;
    }
};

} // namespace ecs
