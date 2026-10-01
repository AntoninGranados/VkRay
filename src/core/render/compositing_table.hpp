#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/ecs/components/compositing.hpp"

namespace ecs {
class Registry;
}

namespace CompositingTable {
inline const std::string kType = "compositing";
inline constexpr int kVersion = 1;

int slotFor(const std::filesystem::path& path);
void generateDispatch();

std::vector<ecs::CompositingPassEntry>& passes(ecs::Registry& registry);
int pack(ecs::CompositingPassEntry& pass, std::vector<float>& params);
int totalDispatchCount(ecs::Registry& registry);
} // namespace CompositingTable
