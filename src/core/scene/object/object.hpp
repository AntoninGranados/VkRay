#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

#include "material.hpp"


struct GpuSphere {
    alignas(16) glm::vec3 center;
    float radius;
    MaterialHandle materialHandle;
};

struct GpuPlane {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 normal;
    MaterialHandle materialHandle;
};

struct GpuBox {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    MaterialHandle materialHandle;
};

struct GpuQuad {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 u;
    alignas(16) glm::vec3 v;
    alignas(16) glm::vec3 normal;
    MaterialHandle materialHandle;
};

struct GpuBvhChild {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    uint32_t index;
};

struct GpuBvhNode {
    GpuBvhChild children[2];
    uint32_t firstTriangle;
    uint32_t triangleCount;
};

struct GpuMesh {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    uint32_t indexOffset;
    uint32_t triangleCount;
    uint32_t bvhOffset;
    uint32_t bvhNodeCount;
    float aabbMinX, aabbMinY, aabbMinZ;
    float aabbMaxX, aabbMaxY, aabbMaxZ;
    MaterialHandle materialHandle;
    uint32_t smoothShading;
    uint32_t hasVertexColor;
};


// Gizmo motion limiting to avoid large jumps when manipulating objects
constexpr float MAX_GIZMO_LINEAR_SPEED   = 50.0f;                // world units per second
constexpr float MAX_GIZMO_SCALE_SPEED    = 50.0f;                // scale units per second
constexpr float MAX_GIZMO_ANGULAR_SPEED  = glm::radians(180.0f); // radians per second

inline float maxStepPerFrame(float speed) {
    const float dt = ImGui::GetIO().DeltaTime;
    return speed > 0.0f && dt > 0.0f ? speed * dt : 0.0f;
}

inline glm::vec3 clampVecDelta(const glm::vec3& delta, float maxLength) {
    if (maxLength <= 0.0f) return glm::vec3(0.0f);
    const float len = glm::length(delta);
    if (len <= maxLength) return delta;
    return delta * (maxLength / len);
}

inline glm::vec3 clampVecDeltaPerAxis(const glm::vec3& delta, float maxDelta) {
    if (maxDelta <= 0.0f) return glm::vec3(0.0f);
    return glm::clamp(delta, glm::vec3(-maxDelta), glm::vec3(maxDelta));
}

inline float clampScalarDelta(float delta, float maxDelta) {
    if (maxDelta <= 0.0f) return 0.0f;
    return glm::clamp(delta, -maxDelta, maxDelta);
}

// Objects
enum class ObjectType : int {
    None   = 0,
    Sphere = 1,
    Plane  = 2,
    Box    = 3,
    Quad   = 4,
    Mesh   = 5,
    Camera = 6,
};

struct ObjectHandle {
    ObjectType type;
    int id;
};

struct GpuObjectHeader {
    uint32_t objectCount;
};

struct GpuLight {
    int objectId;
    float area;
    float pdfA;
};

struct GpuLightHeader {
    float totalArea;
};

inline bool isInvalid(glm::mat4 mat) {
    bool invalid = false;
    for (size_t i = 0; i < 4; i++) {
        const glm::vec4 col = mat[i];
        invalid |= glm::any(glm::isnan(col)) || glm::any(glm::isinf(col));
    }
    return invalid;
}
