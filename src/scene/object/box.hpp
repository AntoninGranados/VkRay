#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuBox {
    alignas(16) glm::vec3 cornerMin;
    alignas(16) glm::vec3 cornerMax;
    Material mat;
};

class Box: public Object {
public:
    Box(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, Material mat);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI() override;
    
    GpuBox getStruct();
    ObjectType getType() override { return ObjectType::BOX; };

private:
    std::string name;
    GpuBox box;

    glm::vec3 cornerMin;
    glm::vec3 cornerMax;
    Material mat;
};
