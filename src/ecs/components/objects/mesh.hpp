#pragma once

#include "../../../scene/asset/mesh.hpp"

namespace ecs {

struct MeshRef {
    MeshHandle handle = 0;

    void setHandle(MeshHandle newHandle) {
        handle = newHandle;
    }
};

} // namespace ecs
