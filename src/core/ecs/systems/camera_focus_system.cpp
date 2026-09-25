#include "camera_focus_system.hpp"

#include "core/camera/camera.hpp"
#include "core/ecs/components/camera.hpp"

namespace ecs {

void cameraFocusSystem(Registry& registry) {
    for (const Entity e : registry.storage(Camera).entities()) {
        Component& c = registry.get(e, Camera);
        if (c.get<Entity>("focus_target") == Entity{}) continue;
        c.set<float>("focal_distance", resolveFocusDistance(registry, e));
    }
}

} // namespace ecs
