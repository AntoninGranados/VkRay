#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../camera.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

// Raytracing
struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};
Ray getRay(const glm::vec2 &mousePos, const glm::vec2 &screenSize, const Camera &camera);

enum MaterialType {
    LAMBERTIAN = 0,
    METAL,
    DIELECTRIC,
    EMISSIVE,
    ANIMATED,
};

struct Material {
    MaterialType type;
    alignas(16) glm::vec3 albedo;
    float fuzz;
    float refraction_index;
    float intensity;
};

// Objects
enum ObjectType {
    NONE,
    SPHERE,
    PLANE,
    BOX,
};

struct GpuObject {
    ObjectType type;
    int id;
};

class Object {
public:
    Object() {};
    virtual float rayIntersection(const Ray &ray) = 0;
    virtual bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) = 0;
    virtual bool drawUI() = 0;
    
    void getStruct(void) {};
    virtual ObjectType getType() = 0;

private:
    std::string name;
};

bool isnan4x4(glm::mat4 mat);
bool drawMaterialUI(Material &mat);
