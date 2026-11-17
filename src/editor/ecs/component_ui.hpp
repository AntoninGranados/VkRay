#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"

namespace ecs {

class ComponentInspector;

class ComponentUi {
public:
    static void draw(Registry& registry, Entity entity);
    static void registerInspector(const ComponentType& type, std::unique_ptr<ComponentInspector> inspector);

private:
    static std::unordered_map<std::string, std::unique_ptr<ComponentInspector>>& inspectors();
    static ComponentInspector* findInspector(const ComponentType& type);
    static const std::vector<const ComponentType*>& drawOrder();
    static std::vector<const ComponentType*> buildDrawOrder();
    static std::ptrdiff_t groupRank(const ComponentType* type);
    static void registerBuiltinInspectors();

    static bool drawComponent(Component& component, Registry& registry, Entity entity);
};

} // namespace ecs
