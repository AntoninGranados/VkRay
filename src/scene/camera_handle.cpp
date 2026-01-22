#include "camera_handle.hpp"

#include <cmath>

CameraHandle::CameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov):
    Object(name), position(position), direction(glm::normalize(direction)), fov(fov) {
}

glm::mat4 CameraHandle::getProjection(GLFWwindow* window) const {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    float fovY = glm::radians(fov);

    glm::mat4 proj = glm::perspective(fovY, aspect, 1e-4f, 1e4f);
    return proj;
}

float CameraHandle::rayIntersection(const Ray &ray) {
    const float radius = selectionRadius;
    const glm::vec3 p = position - ray.origin;
    const float dp = glm::dot(ray.dir, p);
    const float c = glm::dot(p, p) - radius * radius;
    const float delta = dp * dp - c;
    if (delta < 0.0f) return -1.0f;

    float t1 = dp - std::sqrt(delta);
    if (t1 >= 0.0f) return t1;
    float t2 = dp + std::sqrt(delta);
    return t2 >= 0.0f ? t2 : -1.0f;
}

void drawWireframe(const glm::mat4 &view, const glm::mat4 &proj) {
    
}

bool CameraHandle::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    bool updated = false;
    glm::vec3 dir = getDirection();

    glm::vec3 right = glm::normalize(glm::cross(dir, getUp()));
    glm::vec3 camUpBasis = glm::normalize(glm::cross(right, dir));

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right, 0.0f);
    model[1] = glm::vec4(camUpBasis, 0.0f);
    model[2] = glm::vec4(dir, 0.0f);
    model[3] = glm::vec4(position, 1.0f);

    glm::mat4 delta(1.0f);
    if (manipulationEnabled) {
        if (ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(proj),
            ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE,
            ImGuizmo::MODE::WORLD,
            glm::value_ptr(model),
            glm::value_ptr(delta)
        )) {
            if (isInvalid(model) || isInvalid(delta)) return false;
            position = glm::vec3(model[3]);
            direction = glm::normalize(glm::vec3(model[2]));
            updated = true;
        }
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    const glm::mat4 viewProj = proj * view;

    const glm::vec3 camPos = position;
    const glm::vec3 camDir = dir;
    const glm::vec3 camRight = glm::normalize(glm::cross(camDir, getUp()));
    const glm::vec3 camUp = glm::normalize(glm::cross(camRight, camDir));

    const float aspect = 16.0f / 9.0f;
    const float fovY = glm::radians(getFov());
    const float nearDist = 0.5f;
    const float farDist = 2.5f;
    const float nearHalfH = tanf(fovY * 0.5f) * nearDist;
    const float nearHalfW = nearHalfH * aspect;
    const float farHalfH = tanf(fovY * 0.5f) * farDist;
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

    auto clipLineToPlane = [](glm::vec4 &a, glm::vec4 &b, float da, float db) -> bool {
        if (da >= 0.0f && db >= 0.0f) return true;
        if (da < 0.0f && db < 0.0f) return false;
        float t = da / (da - db);
        glm::vec4 p = a + t * (b - a);
        if (da < 0.0f) a = p; else b = p;
        return true;
    };

    auto clipLine = [&](glm::vec4 &a, glm::vec4 &b) -> bool {
        if (!clipLineToPlane(a, b,  a.x + a.w,  b.x + b.w)) return false;
        if (!clipLineToPlane(a, b, -a.x + a.w, -b.x + b.w)) return false;
        if (!clipLineToPlane(a, b,  a.y + a.w,  b.y + b.w)) return false;
        if (!clipLineToPlane(a, b, -a.y + a.w, -b.y + b.w)) return false;
        if (!clipLineToPlane(a, b,  a.z,        b.z)) return false;
        if (!clipLineToPlane(a, b,  a.w - a.z,  b.w - b.z)) return false;
        return true;
    };

    auto toScreen = [&](const glm::vec4 &p) -> ImVec2 {
        const glm::vec3 ndc = glm::vec3(p) / p.w;
        const float x = (ndc.x * 0.5f + 0.5f) * windowSize.x + windowPos.x;
        const float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * windowSize.y + windowPos.y;
        return ImVec2(x, y);
    };

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImU32 lineColor = selected ? IM_COL32(255, 128, 16, 255) : IM_COL32(0, 0, 0, 255);
    const float thickness = 2.0f;

    auto drawClipped = [&](glm::vec4 a, glm::vec4 b) {
        if (!clipLine(a, b)) return;
        drawList->AddLine(toScreen(a), toScreen(b), lineColor, thickness);
    };

    const int edges[4][2] = { {0, 1}, {1, 2}, {2, 3}, {3, 0} };
    for (const auto &edge : edges) {
        drawClipped(clipNear[edge[0]], clipNear[edge[1]]);
        drawClipped(clipFar[edge[0]], clipFar[edge[1]]);
    }
    for (int i = 0; i < 4; i++) {
        drawClipped(clipNear[i], clipFar[i]);
    }

    const float apertureRadius = getAperture() * 0.5f;
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

    return updated;
}

bool CameraHandle::drawUI(std::vector<Material> &materials) {
    bool updated = false;
    (void)materials;

    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##CameraHandlePos", glm::value_ptr(position), 0.01f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Direction:");
    ImGui::PushItemWidth(-FLT_MIN);
    glm::vec3 newDirection = direction;
    if (ImGui::DragFloat3("##CameraHandleDirection", glm::value_ptr(newDirection), 0.01f)) {
        if (glm::length(newDirection) > 1e-4f)
            direction = glm::normalize(newDirection);
        updated = true;
    }
    ImGui::PopItemWidth();

    ImGui::Text("Fov:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##CameraHandleFov", &fov, 0.1f, 1.0f, 160.0f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Aperture:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##CameraHandleAperture", &aperture, 0.01f, 0.0f, 5.0f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Focus Depth:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat("##CameraHandleFocusDepth", &focusDepth, 0.1f, 0.0f, 100.0f))
        updated = true;
    ImGui::PopItemWidth();

    return updated;
}
