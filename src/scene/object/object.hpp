#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../camera.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

// Gizmo motion limiting to avoid large jumps when manipulating objects
constexpr float MAX_GIZMO_LINEAR_SPEED   = 50.0f;                // world units per second
constexpr float MAX_GIZMO_SCALE_SPEED    = 50.0f;                // scale units per second
constexpr float MAX_GIZMO_ANGULAR_SPEED  = glm::radians(180.0f); // radians per second

inline float maxStepPerFrame(float speed) {
    const float dt = ImGui::GetIO().DeltaTime;
    return speed > 0.0f && dt > 0.0f ? speed * dt : 0.0f;
}

inline glm::vec3 clampVecDelta(const glm::vec3 &delta, float maxLength) {
    if (maxLength <= 0.0f) return glm::vec3(0.0f);
    const float len = glm::length(delta);
    if (len <= maxLength) return delta;
    return delta * (maxLength / len);
}

inline glm::vec3 clampVecDeltaPerAxis(const glm::vec3 &delta, float maxDelta) {
    if (maxDelta <= 0.0f) return glm::vec3(0.0f);
    return glm::clamp(delta, glm::vec3(-maxDelta), glm::vec3(maxDelta));
}

inline float clampScalarDelta(float delta, float maxDelta) {
    if (maxDelta <= 0.0f) return 0.0f;
    return glm::clamp(delta, -maxDelta, maxDelta);
}

// Raytracing
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};
Ray getRay(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const Camera &camera);

enum MaterialType {
    Lambertian = 0,
    Metal,
    Dielectric,
    Emissive,
    Animated,
};

struct Material {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

// Objects
enum class ObjectType : int {
    None,
    Sphere,
    Plane,
    Box,
};

struct GpuObject {
    ObjectType type;
    int id;
};

class Object {
public:
    Object(std::string name): name(name) {};
    virtual float rayIntersection(const Ray &ray) = 0;
    virtual bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) = 0;
    virtual bool drawUI() = 0;
    
    void getStruct(void) {};
    std::string getName() { return name; }
    void setName(std::string newName) { name = newName; }
    virtual ObjectType getType() = 0;

protected:
    std::string name;
};

bool isInvalid(glm::mat4 mat);
bool drawMaterialUI(Material &mat);
