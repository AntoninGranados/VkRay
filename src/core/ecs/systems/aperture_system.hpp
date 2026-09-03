#pragma once

#include <filesystem>

#include "core/ecs/registry.hpp"

namespace ecs {

struct ApertureState {
    int  lastBlades      = -1;
    float lastRotation   = -1.0f;
    std::filesystem::path lastPath;
    bool defaultUploaded = false;
};

void apertureSystem(Registry& registry);

} // namespace ecs
