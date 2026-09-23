#pragma once

// GPU-side mirror of the scene primitives; see core/ecs/components/geometry.hpp for their ECS-side definitions.

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

static constexpr int kMaterialPayloadSize = 12;

struct GpuMaterial {
    int      type;
    uint32_t base;
};

struct GpuMotionSample {
    alignas(16) glm::vec4 rotation;
    alignas(16) glm::vec3 translation;
    alignas(16) glm::vec3 scale;
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
    uint32_t indexOffset;
    uint32_t triangleCount;
    uint32_t bvhOffset;
    uint32_t bvhNodeCount;
    float aabbMinX, aabbMinY, aabbMinZ;
    float aabbMaxX, aabbMaxY, aabbMaxZ;
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

struct GpuObject {
    ObjectType type;
    uint32_t id;
    uint32_t materialSlot;
    uint32_t motionOffset;
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

inline bool isInvalid(glm::mat4 matrix) {
    bool invalid = false;
    for (size_t i = 0; i < 4; i++) {
        const glm::vec4 col = matrix[i];
        invalid |= glm::any(glm::isnan(col)) || glm::any(glm::isinf(col));
    }
    return invalid;
}
