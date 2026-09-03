#include "camera_drawing_system.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "imgui/imgui.h"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/parameters/parameters.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"

namespace ecs {

namespace {

void drawCameraGizmo(Registry& registry, Entity e, Entity activeCamera, ImDrawList* drawList, ImVec2 windowPos, ImVec2 windowSize) {
    const Component& c = registry.get(e, Camera);
    const Component& t = registry.get(e, Transform);

    const glm::quat rot = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
    glm::vec3 dir = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
    const glm::vec3 up = glm::normalize(rot * glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(dir) < 1e-6f) dir = glm::vec3(0.0f, 0.0f, -1.0f);

    const float aspect = windowSize.y > 0.0f ? (windowSize.x / windowSize.y) : 1.0f;
    const float fov = glm::radians(c.get<float>("fov"));

    const glm::mat4 view = getView(registry, activeCamera);
    const glm::mat4 proj = getProjection(registry, activeCamera, aspect);
    const glm::mat4 viewProj = proj * view;

    const glm::vec3 camPos = t.get<glm::vec3>("position");
    const glm::vec3 camDir = dir;
    const glm::vec3 camRight = glm::normalize(glm::cross(camDir, up));
    const glm::vec3 camUp = glm::normalize(glm::cross(camRight, camDir));

    const float nearDist = 0.5f;
    const float farDist = 1.5f;
    const float nearHalfH = tanf(fov * 0.5f) * nearDist;
    const float nearHalfW = nearHalfH * aspect;
    const float farHalfH = tanf(fov * 0.5f) * farDist;
    const float farHalfW = farHalfH * aspect;

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

    const glm::vec4 clipNear[4] = {
        viewProj * glm::vec4(nearCorners[0], 1.0f),
        viewProj * glm::vec4(nearCorners[1], 1.0f),
        viewProj * glm::vec4(nearCorners[2], 1.0f),
        viewProj * glm::vec4(nearCorners[3], 1.0f),
    };
    const glm::vec4 clipFar[4] = {
        viewProj * glm::vec4(farCorners[0], 1.0f),
        viewProj * glm::vec4(farCorners[1], 1.0f),
        viewProj * glm::vec4(farCorners[2], 1.0f),
        viewProj * glm::vec4(farCorners[3], 1.0f),
    };

    auto clipLineToPlane = [](glm::vec4& a, glm::vec4& b, float da, float db) -> bool {
        if (da >= 0.0f && db >= 0.0f) return true;
        if (da < 0.0f && db < 0.0f) return false;
        float t = da / (da - db);
        glm::vec4 p = a + t * (b - a);
        if (da < 0.0f) a = p; else b = p;
        return true;
    };

    auto clipLine = [&](glm::vec4& a, glm::vec4& b) -> bool {
        if (!clipLineToPlane(a, b,  a.x + a.w,  b.x + b.w)) return false;
        if (!clipLineToPlane(a, b, -a.x + a.w, -b.x + b.w)) return false;
        if (!clipLineToPlane(a, b,  a.y + a.w,  b.y + b.w)) return false;
        if (!clipLineToPlane(a, b, -a.y + a.w, -b.y + b.w)) return false;
        if (!clipLineToPlane(a, b,  a.z,        b.z)) return false;
        if (!clipLineToPlane(a, b,  a.w - a.z,  b.w - b.z)) return false;
        return true;
    };

    auto toScreen = [&](const glm::vec4& p) -> ImVec2 {
        const glm::vec3 ndc = glm::vec3(p) / p.w;
        const float x = windowPos.x + (ndc.x * 0.5f + 0.5f) * windowSize.x;
        const float y = windowPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * windowSize.y;
        return ImVec2(x, y);
    };

    const std::optional<ecs::Entity> selectedEntity = Editor::getSelectedEntity();
    const bool isSelected = selectedEntity.has_value() && e == *selectedEntity;
    const ImU32 lineColor = isSelected ? IM_COL32(255, 128, 16, 255) : IM_COL32(0, 0, 0, 255);
    const float distToCamera = glm::length(registry.get(activeCamera, ecs::Transform).get<glm::vec3>("position") - camPos);
    const float thickness = std::clamp(4.0f / (0.15f * distToCamera + 1.0f), 0.75f, 4.0f);

    auto drawClipped = [&](glm::vec4 a, glm::vec4 b) {
        if (!clipLine(a, b)) return;
        drawList->AddLine(toScreen(a), toScreen(b), lineColor, thickness);
    };

    const int edges[4][2] = { {0, 1}, {1, 2}, {2, 3}, {3, 0} };
    for (const auto& edge : edges) {
        drawClipped(clipNear[edge[0]], clipNear[edge[1]]);
        drawClipped(clipFar[edge[0]], clipFar[edge[1]]);
    }
    for (int i = 0; i < 4; i++) {
        drawClipped(clipNear[i], clipFar[i]);
    }

    float apertureRadius = 0.0f;
    if (registry.has(e, ThinLens)) {
        const auto& tl = registry.get(e, ThinLens);
        const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
        apertureRadius = lensRadiusFromFStop(tl.get<float>("focal_length") / sensorWidth, tl.get<float>("f_stop"));
    }
    if (apertureRadius > 1e-4f) {
        const int ringSegments = 32;
        glm::vec3 prevPoint = camPos + camRight * apertureRadius;
        glm::vec4 prevClip = viewProj * glm::vec4(prevPoint, 1.0f);
        for (int i = 1; i <= ringSegments; i++) {
            float angle = (2.0f * glm::pi<float>()) * (static_cast<float>(i) / ringSegments);
            glm::vec3 point = camPos
                + camRight * (cosf(angle) * apertureRadius)
                + camUp * (sinf(angle) * apertureRadius);
            glm::vec4 clip = viewProj * glm::vec4(point, 1.0f);
            drawClipped(prevClip, clip);
            prevClip = clip;
        }
    }
}

} // namespace

void cameraDrawingSystem(Registry& registry) {
    if (Core::getRenderMode() != RenderMode::Preview) return;    // don't draw the cameras when rendering

    auto& cameras = registry.storage(Camera);
    auto& transforms = registry.storage(Transform);

    if (ImGui::GetCurrentContext() == nullptr) return;

    ImVec2 windowPos = Editor::getViewportPos();
    ImVec2 windowSize = Editor::getViewportSize();
    if (windowSize.x == 0.0f || windowSize.y == 0.0f) return;

    ImDrawList* drawList = Editor::getViewportDrawList();
    if (!drawList) return;
    drawList->PushClipRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), true);

    const ecs::Entity activeCamera = Core::getScene().getCamera();
    const ecs::Entity defaultCamera = Core::getScene().getDefaultCamera();
    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        if (e == activeCamera || e == defaultCamera) continue;
        drawCameraGizmo(registry, e, activeCamera, drawList, windowPos, windowSize);
    }

    drawList->PopClipRect();
}

} // namespace ecs
