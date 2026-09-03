#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

struct GpuMaterial;

class MaterialTable {
public:
    static bool pack(ecs::Registry& registry, ecs::Entity entity, GpuMaterial& gpu, std::vector<float>& params);
    static void generateGlsl();

private:
    struct Entry {
        const ecs::ComponentType* type;
        std::function<std::vector<float>(ecs::Component&)> customValues;
    };

    static const std::vector<Entry>& entries();

    static std::string glslMacroName(const ecs::ComponentType& type, const ecs::ComponentField& field);
    static bool isPackableField(const ecs::ComponentField& field);
    static bool hasAlbedoField(const ecs::ComponentType& type);
};
