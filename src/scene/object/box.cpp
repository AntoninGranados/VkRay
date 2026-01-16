#include "box.hpp"

#include <cmath>
#include <limits>

Box::Box(std::string name, glm::mat4 transform, MaterialHandle materialHandle):
    Object(name), transform(transform), materialHandle(materialHandle) {
}

float Box::rayIntersection(const Ray &ray) {
    glm::mat4 invTransform = glm::inverse(transform);
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.dir, 0.0f));
    Ray localRay{ localOrigin, localDir };

    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        const float dir = localRay.dir[i];
        if (std::abs(dir) < 1e-8f) {
            if (localRay.origin[i] < -1.0f || localRay.origin[i] > 1.0f) return -1.0f;
            continue;
        }

        const float invD = 1.0f / dir;
        float t0 = (-1.0f - localRay.origin[i]) * invD;
        float t1 = ( 1.0f - localRay.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);

        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return -1.0f;
    }

    if (tmax < 0.0f) return -1.0f;
    return tmin >= 0.0f ? tmin : tmax;
}

bool Box::drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) {
    glm::vec3 currentPos, currentRot, currentScale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(currentPos),
        glm::value_ptr(currentRot),
        glm::value_ptr(currentScale)
    );

    glm::mat4 model = transform;
    if (ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        ImGuizmo::OPERATION::TRANSLATE | ImGuizmo::OPERATION::ROTATE | ImGuizmo::OPERATION::SCALE,
        ImGuizmo::MODE::WORLD, 
        glm::value_ptr(model)
    )) {
        if (isInvalid(model)) return false;

        glm::vec3 targetPos, targetRot, targetScale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            glm::value_ptr(targetPos),
            glm::value_ptr(targetRot),
            glm::value_ptr(targetScale)
        );

        const float maxStep = maxStepPerFrame(MAX_GIZMO_LINEAR_SPEED);
        targetPos = currentPos + clampVecDelta(targetPos - currentPos, maxStep);
        const float scaleSpeed = MAX_GIZMO_SCALE_SPEED * 0.2f;
        const float maxScaleStep = maxStepPerFrame(scaleSpeed);
        targetScale = currentScale + clampVecDeltaPerAxis(targetScale - currentScale, maxScaleStep);
        targetScale = glm::max(targetScale, glm::vec3(0.001f));

        ImGuizmo::RecomposeMatrixFromComponents(
            glm::value_ptr(targetPos),
            glm::value_ptr(targetRot),
            glm::value_ptr(targetScale),
            glm::value_ptr(transform)
        );
        return true;
    }

    return false;
}

bool Box::drawUI(std::vector<Material> &materials) {
    bool updated = false;
    glm::vec3 translation, rotation, scale;
    ImGuizmo::DecomposeMatrixToComponents(
        glm::value_ptr(transform),
        glm::value_ptr(translation),
        glm::value_ptr(rotation),
        glm::value_ptr(scale)
    );

    ImGui::Text("Position:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Position", glm::value_ptr(translation), 0.01f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Rotation:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation), 0.1f))
        updated = true;
    ImGui::PopItemWidth();

    ImGui::Text("Scale:");
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.01f, 0.001f, 1000.0f))
        updated = true;
    ImGui::PopItemWidth();

    if (updated) {
        ImGuizmo::RecomposeMatrixFromComponents(
            glm::value_ptr(translation),
            glm::value_ptr(rotation),
            glm::value_ptr(scale),
            glm::value_ptr(transform)
        );
    }
    
    updated |= drawMaterialUI(materials[materialHandle]);
    return updated;
}

float Box::getArea() {
    glm::vec3 sx = glm::vec3(transform[0]);
    glm::vec3 sy = glm::vec3(transform[1]);
    glm::vec3 sz = glm::vec3(transform[2]);
    float lx = glm::length(sx);
    float ly = glm::length(sy);
    float lz = glm::length(sz);
    float wx = 2.0f * lx;
    float wy = 2.0f * ly;
    float wz = 2.0f * lz;
    return 2.0f * (wx * wy + wy * wz + wx * wz);
}

GpuBox Box::getStruct() {
    box.transform = transform;
    box.invTransform = glm::inverse(transform);
    box.materialHandle = materialHandle;
    return box;
}
