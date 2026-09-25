#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/ecs/entity.hpp"

namespace ecs { class Registry; }

namespace SkyTable {
    inline const std::string kType = "sky";
    inline constexpr int kVersion = 1;

    int slotFor(const std::filesystem::path& path);
    void generateDispatch();
    void pack(ecs::Registry& registry, ecs::Entity environment, std::vector<float>& params);
}
