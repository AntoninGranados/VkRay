#include "camera_system.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"
#include "scene/scene.hpp"
#include "engine/engine.hpp"

namespace ecs {

void cameraDrawingSystem(Registry& registry, AppContext& ctx) {
    if (ctx.renderState->renderMode != RenderMode::Preview) return;    // don't draw the cameras when rendering

    auto& cameras = registry.storage<ecs::CameraObject>();
    auto& transforms = registry.storage<ecs::Transform>();

    if (ImGui::GetCurrentContext() == nullptr) return;

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        const auto& c = cameras.get(e);
        const auto& t = transforms.get(e);
        if (c.isPreview) continue;

        glm::vec3 dir = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        const glm::vec3 up = glm::normalize(t.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::length(dir) < 1e-6f) dir = glm::vec3(0.0f, 0.0f, -1.0f);

        const float aspect = windowSize.y > 0.0f ? (windowSize.x / windowSize.y) : 1.0f;
        const float fov = glm::radians(c.fov);

        const Camera& activeCamera = ctx.scene->getCamera();
        const glm::mat4 view = activeCamera.getView();
        const glm::mat4 proj = activeCamera.getProjection(ctx.engine->getWindow().get());
        const glm::mat4 viewProj = proj * view;
    
        const glm::vec3 camPos = t.position;
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
    
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 lineColor = (ctx.scene->getSelectedEntity() && e == *ctx.scene->getSelectedEntity()) ? IM_COL32(255, 128, 16, 255) : IM_COL32(0, 0, 0, 255);
        const float distToCamera = glm::length(activeCamera.getPosition() - camPos);
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
    
        const float apertureRadius = c.aperture * 0.5f;
        if (apertureRadius > 1e-4f) {
            const int ringSegments = 32;
            glm::vec3 prevPoint = camPos + camRight * apertureRadius;
            glm::vec4 prevClip = viewProj * glm::vec4(prevPoint, 1.0f);
            for (int i = 1; i <= ringSegments; i++) {
                float angle = (2.0f * 3.14159265f) * (static_cast<float>(i) / ringSegments);
                glm::vec3 point = camPos
                    + camRight * (cosf(angle) * apertureRadius)
                    + camUp * (sinf(angle) * apertureRadius);
                glm::vec4 clip = viewProj * glm::vec4(point, 1.0f);
                drawClipped(prevClip, clip);
                prevClip = clip;
            }
        }
    }
}

void cameraPreUpdateSystem(Registry& registry, AppContext& ctx) {
    auto& cameras = registry.storage<ecs::CameraObject>();
    auto& transforms = registry.storage<ecs::Transform>();
    constexpr float previewViewportScale = 0.8f;
    
    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        
        ecs::CameraObject& c = cameras.get(e);
        if (ctx.renderState->renderMode != RenderMode::Preview && !c.isPreview) {
            c.setPreview(true);
            c.setPreviewJustSet(true);
            *ctx.restartRender = true;
        }

        if (!c.isPreview) continue;

        ecs::Transform& t = transforms.get(e);
        Camera& camera = *ctx.camera;

        const float dist = glm::max(0.1f, glm::length(camera.getTarget() - camera.getPosition()));
        const glm::vec3 dir = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));

        camera.setPosition(t.position);
        camera.setTarget(t.position + dir * dist);
        float fov = c.fov;
        if (ctx.renderState->renderMode == RenderMode::Preview) {
            const float baseFovRad = glm::radians(c.fov);
            const float previewFovRad = 2.0f * atanf(tanf(baseFovRad * 0.5f) / previewViewportScale);
            fov = glm::degrees(previewFovRad);
        }
        camera.setFov(fov);
        camera.setAperture(c.aperture);
        camera.setFocusDepth(c.focusDepth);
        break;
    }
}

void cameraPostUpdateSystem(Registry& registry, AppContext& ctx) {
    auto& cameras = registry.storage<ecs::CameraObject>();
    auto& transforms = registry.storage<ecs::Transform>();
    bool escapePressed = glfwGetKey(ctx.engine->getWindow().get(), GLFW_KEY_ESCAPE);
    Camera& camera = *ctx.camera;
    constexpr float previewViewportScale = 0.8f;

    for (const auto& e : cameras.entities()) {
        ecs::CameraObject& c = cameras.get(e);
        if (!c.isPreview) continue;

        if (escapePressed) {
            c.setPreview(false);
            *ctx.restartRender = true;
            ctx.camera->setAperture(0.0f);
            continue;
        }

        if (c.previewJustSet) {
            c.previewJustSet = false;
            continue;
        }

        if (c.updated) {
            float fov = c.fov;
            if (ctx.renderState->renderMode == RenderMode::Preview) {
                const float baseFovRad = glm::radians(c.fov);
                const float previewFovRad = 2.0f * atanf(tanf(baseFovRad * 0.5f) / previewViewportScale);
                fov = glm::degrees(previewFovRad);
            }
            camera.setFov(fov);
            camera.setAperture(c.aperture);
            camera.setFocusDepth(c.focusDepth);
            c.updated = false;
            continue;
        }

        if (!transforms.has(e)) continue;
        ecs::Transform& t = transforms.get(e);

        if (t.updated) {
            const float dist = glm::max(0.1f, glm::length(camera.getTarget() - camera.getPosition()));
            const glm::vec3 dir = glm::normalize(t.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
            camera.setPosition(t.position);
            camera.setTarget(t.position + dir * dist);
            t.updated = false;
            continue;
        }

        float fov = camera.getFov();
        if (ctx.renderState->renderMode == RenderMode::Preview) {
            const float previewFovRad = glm::radians(camera.getFov());
            const float baseFovRad = 2.0f * atanf(tanf(previewFovRad * 0.5f) * previewViewportScale);
            fov = glm::degrees(baseFovRad);
        }
        c.setFov(fov);
        c.setAperture(camera.getAperture());
        c.setFocusDepth(camera.getFocusDepth());
        t.setPosition(camera.getPosition());
        t.setRotation(glm::quatLookAt(glm::normalize(camera.getDirection()), camera.getUp()));
    }
}

} // namespace ecs
