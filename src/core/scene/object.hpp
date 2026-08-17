#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

static constexpr int kMaterialPayloadSize = 12;

struct GpuMaterial {
    int   type;
    float payload[kMaterialPayloadSize];
};

struct GpuSphere {
    alignas(16) glm::vec3 center;
    float radius;
    uint32_t materialHandle;
};

struct GpuPlane {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 normal;
    uint32_t materialHandle;
};

struct GpuBox {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    uint32_t materialHandle;
};

struct GpuQuad {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 u;
    alignas(16) glm::vec3 v;
    alignas(16) glm::vec3 normal;
    uint32_t materialHandle;
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
    uint32_t materialHandle;
    uint32_t smoothShading;
    uint32_t hasVertexColor;
};


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
