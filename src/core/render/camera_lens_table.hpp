#pragma once

#include <filesystem>
#include <string>

#include "core/ecs/entity.hpp"
#include "core/render_structures.hpp"

namespace ecs { class Registry; }

namespace CameraLensTable {
    inline const std::string kType = "camera_lens";
    inline constexpr int kVersion = 1;
    inline constexpr int kParamCapacity = 16;

    int slotFor(const std::filesystem::path& path);
    void generateDispatch();
    void pack(ecs::Registry& registry, ecs::Entity camera, CameraUBO& ubo);
}
