#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "material.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuSphere {
    alignas(16) glm::vec3 center;
    float radius;
    MaterialHandle materialHandle;
};

class Sphere: public Object {
public:
    Sphere(std::string name, glm::vec3 center, float radius, MaterialHandle materialHandle);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;
    
    GpuSphere getStruct();
    ObjectType getType() override { return ObjectType::Sphere; };

private:
    GpuSphere sphere;

    glm::vec3 center;
    float radius;
    MaterialHandle materialHandle;
};
