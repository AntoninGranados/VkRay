#include "camera_system.hpp"
#include "core/ecs/entity.hpp"

#include <glm/glm.hpp>

#include "core/camera/camera.hpp"
#include "core/core.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry) {
    const ecs::Entity camera = Core::getScene().getCamera();
    if (!registry.has(camera, Camera) || !registry.has(camera, ThinLens)) return;

    Component& c = registry.get(camera, Camera);
    Component& thinLens = registry.get(camera, ThinLens);
    auto& sync = thinLens.payload<ThinLensSync>("sync");

    const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
    const float fovDeg = c.get<float>("fov");
    const float focalLengthMm = thinLens.get<float>("focal_length");

    if (!sync.initialized) {
        const float newFov = fovFromFocalLength(focalLengthMm / sensorWidth);
        c.set<float>("fov", newFov);
        sync.lastFov = newFov;
        sync.lastFocalLength = focalLengthMm;
        sync.initialized = true;
        return;
    }

    if (glm::abs(focalLengthMm - sync.lastFocalLength) > 1e-4f) {
        const float newFov = fovFromFocalLength(focalLengthMm / sensorWidth);
        c.set<float>("fov", newFov);
        sync.lastFov = newFov;
        sync.lastFocalLength = focalLengthMm;
    } else if (glm::abs(fovDeg - sync.lastFov) > 1e-4f) {
        const float newFocalLength = focalLengthFromFov(fovDeg) * sensorWidth;
        thinLens.set<float>("focal_length", newFocalLength);
        sync.lastFov = fovDeg;
        sync.lastFocalLength = newFocalLength;
    }
}

} // namespace ecs
