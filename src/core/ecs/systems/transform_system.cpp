#include "transform_system.hpp"

#include "../components/transform.hpp"

namespace ecs {

void transformSystem(Registry& registry) {
    auto& transforms = registry.storage<ecs::Transform>();
    for (const auto& e : transforms.entities()) {
        Transform& t = transforms.get(e);
        t.updateLocal();
    }
}

} // namespace ecs
