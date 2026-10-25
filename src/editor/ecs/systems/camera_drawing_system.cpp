#include "camera_drawing_system.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/fields/parameters.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/overlay_drawer.hpp"

namespace ecs {

namespace {

void drawCameraGizmo(Registry& registry, Entity e, float aspect, OverlayDrawer& drawer) {
    const Component& c = registry.get(e, Camera);
    const Component& t = registry.get(e, Transform);

    const glm::quat rot = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
    glm::vec3 dir = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
    const glm::vec3 up = glm::normalize(rot * glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(dir) < 1e-6f) dir = glm::vec3(0.0f, 0.0f, -1.0f);

    const float sensorWidth = c.get<float>("sensor_width");
    const CameraFrustum frustum = computeFrustum(registry, e, aspect);
    const bool orthographic = frustum.orthographic;

    const glm::vec3 camPos = t.get<glm::vec3>("position");
    const glm::vec3 camDir = dir;
    const glm::vec3 camRight = glm::normalize(glm::cross(camDir, up));
    const glm::vec3 camUp = glm::normalize(glm::cross(camRight, camDir));

    const float nearDist = 0.5f;
    const float farDist = 1.5f;
    const float nearHalfW = orthographic ? frustum.half.x : frustum.half.x * nearDist;
    const float nearHalfH = orthographic ? frustum.half.y : frustum.half.y * nearDist;
    const float farHalfW = orthographic ? frustum.half.x : frustum.half.x * farDist;
    const float farHalfH = orthographic ? frustum.half.y : frustum.half.y * farDist;

    const glm::vec3 nearCenter = camPos + camDir * nearDist;
    const glm::vec3 farCenter = camPos + camDir * farDist;

    const glm::vec3 nearCorners[4] = {
        nearCenter + camUp * nearHalfH - camRight * nearHalfW,
        nearCenter + camUp * nearHalfH + camRight * nearHalfW,
        nearCenter - camUp * nearHalfH + camRight * nearHalfW,
        nearCenter - camUp * nearHalfH - camRight * nearHalfW,
    };
    const glm::vec3 farCorners[4] = {
        farCenter + camUp * farHalfH - camRight * farHalfW,
        farCenter + camUp * farHalfH + camRight * farHalfW,
        farCenter - camUp * farHalfH + camRight * farHalfW,
        farCenter - camUp * farHalfH - camRight * farHalfW,
    };

    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    const bool isSelected = selectedEntity.has_value() && e == *selectedEntity;
    const glm::vec3 lineColor = isSelected ? glm::vec3(1.0f, 128.0f / 255.0f, 16.0f / 255.0f) : glm::vec3(0.0f);

    const int edges[4][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    for (const auto& edge : edges) {
        drawer.line(nearCorners[edge[0]], nearCorners[edge[1]], lineColor, isSelected);
        drawer.line(farCorners[edge[0]], farCorners[edge[1]], lineColor, isSelected);
    }
    for (int i = 0; i < 4; i++) { drawer.line(nearCorners[i], farCorners[i], lineColor, isSelected); }

    const float apertureRadius =
        orthographic ? 0.0f : lensRadiusFromFStop(c.get<float>("focal_length") / sensorWidth, c.get<float>("f_stop"));
    if (apertureRadius > 1e-4f) {
        const int ringSegments = 32;
        glm::vec3 prevPoint = camPos + camRight * apertureRadius;
        for (int i = 1; i <= ringSegments; i++) {
            float angle = (2.0f * glm::pi<float>()) * (static_cast<float>(i) / ringSegments);
            glm::vec3 point =
                camPos + camRight * (cosf(angle) * apertureRadius) + camUp * (sinf(angle) * apertureRadius);
            drawer.line(prevPoint, point, lineColor, isSelected);
            prevPoint = point;
        }
    }
}

} // namespace

void cameraDrawingSystem(Registry& registry) {
    if (Core::getRenderMode() != RenderMode::Preview) return; // don't draw the cameras when rendering

    auto& cameras = registry.storage(Camera);
    auto& transforms = registry.storage(Transform);

    const glm::ivec2 renderSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const float aspect = renderSize.y > 0 ? static_cast<float>(renderSize.x) / static_cast<float>(renderSize.y) : 1.0f;

    OverlayDrawer& drawer = Editor::getEditorRenderer().getOverlayDrawer();
    const ecs::Entity activeCamera = *Core::getScene().getActiveCamera();
    const ecs::Entity viewportCamera = Editor::getViewportCamera();
    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        if (e == activeCamera || e == viewportCamera) continue;
        drawCameraGizmo(registry, e, aspect, drawer);
    }
}

} // namespace ecs
