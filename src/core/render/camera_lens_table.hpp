#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/ecs/entity.hpp"

namespace ecs {
class Registry;
}

namespace CameraLensTable {
inline const std::string kType = "camera_lens";
inline constexpr int kVersion = 1;

int slotFor(const std::filesystem::path& path);
void generateDispatch();
int pack(ecs::Registry& registry, ecs::Entity camera, std::vector<float>& params);
} // namespace CameraLensTable
