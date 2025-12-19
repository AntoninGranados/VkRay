#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "material.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuBox {
    alignas(16) glm::vec3 cornerMin;
    alignas(16) glm::vec3 cornerMax;
    MaterialHandle materialHandle;
};

class Box: public Object {
public:
    Box(std::string name, glm::vec3 cornerMin, glm::vec3 cornerMax, MaterialHandle materialHandle);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;
    
    float getArea() override;
    GpuBox getStruct();
    ObjectType getType() override { return ObjectType::Box; };

private:
    GpuBox box;

    glm::vec3 cornerMin;
    glm::vec3 cornerMax;
    MaterialHandle materialHandle;
};
