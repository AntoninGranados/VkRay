#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuSphere {
    alignas(16) glm::vec3 center;
    float radius;
    Material mat;
};

class Sphere: public Object {
public:
    Sphere(std::string name, glm::vec3 center, float radius, Material mat);
    float rayIntersection(const Ray &ray) override;
    void drawGuizmo(int &frameCount, const glm::mat4 &view, const glm::mat4 &proj) override;
    void drawUI(int &frameCount) override;
    
    GpuSphere getStruct();
    ObjectType getType() override { return ObjectType::SPHERE; };

private:
    std::string name;
    GpuSphere sphere;

    glm::vec3 center;
    float radius;
    Material mat;
};
