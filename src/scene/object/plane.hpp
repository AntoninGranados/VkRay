#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuPlane {
    alignas(16) glm::vec3 point;
    alignas(16) glm::vec3 normal;
    Material mat;
};

class Plane: public Object {
public:
    Plane(std::string name, glm::vec3 point, glm::vec3 normal, Material mat);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI() override;
    
    GpuPlane getStruct();
    ObjectType getType() override { return ObjectType::Plane; };

private:
    GpuPlane plane;

    glm::vec3 point;
    glm::vec3 normal;
    Material mat;
};
