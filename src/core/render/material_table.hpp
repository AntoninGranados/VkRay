#pragma once

#include <vector>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

struct GpuMaterial;

namespace MaterialTable {
    bool pack(ecs::Registry& registry, ecs::Entity entity, GpuMaterial& gpu, std::vector<float>& params);
    void generateGlsl();
}
