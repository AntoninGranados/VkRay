#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

struct GpuMaterial;

namespace MaterialTable {
    inline const std::string kType = "material";
    inline constexpr int kVersion = 1;

    int slotFor(const std::filesystem::path& path);

    bool pack(ecs::Registry& registry, ecs::Entity entity, GpuMaterial& gpu, std::vector<float>& params);
    void generateGlsl();
    void generateDispatch();
}
