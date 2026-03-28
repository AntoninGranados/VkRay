#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "material.hpp"
#include "../raycast.hpp"
#include "imgui/imgui.h"

class Camera;


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

struct GpuBvhNode {
    alignas(16) glm::vec3 aabbMin;
    alignas(16) glm::vec3 aabbMax;
    uint32_t data0; // left or first triangle
    uint32_t data1; // right or triangle count
    uint32_t isLeaf;
};

#define BVH_childLeft(node)   (node.data0)
#define BVH_childRight(node)  (node.data1)
#define BVH_firstTriangle(node) (node.data0)
#define BVH_triangleCount(node) (node.data1)

struct GpuMesh {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    uint32_t indexOffset;
    uint32_t triangleCount;
    uint32_t bvhOffset;
    uint32_t bvhNodeCount;
    MaterialHandle materialHandle;
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
    None = 0,
    Sphere = 1,
    Plane = 2,
    Aabb = 3,
    Box = 4,
    Mesh = 5,
    Camera = 6,
};

struct ObjectHandle {
    ObjectType type;
    int id;
};

struct GpuObjectHeader {
    uint32_t objectCount;
    int selectedObject;
};

struct GpuLight {
    int objectId;
    float area;
    float pdfA;
};

struct GpuLightHeader {
    float totalArea;
};

class Object {
public:
    Object(std::string name): name(name) {};
    virtual float rayIntersection(const Ray& ray) = 0;
    virtual bool drawGuizmo(const glm::mat4& view, const glm::mat4& proj) = 0;
    virtual bool drawUI(std::vector<Material>& materials) = 0;
    
    virtual float getArea() = 0;
    void getStruct(void) {};
    std::string getName() const { return name; }
    void setName(std::string newName) { name = newName; }
    virtual ObjectType getType() = 0;

protected:
    std::string name;
};

bool isInvalid(glm::mat4 mat);
