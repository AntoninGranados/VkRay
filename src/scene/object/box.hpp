#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "object.hpp"
#include "material.hpp"
#include "imgui/imgui.h"
#include "imgui/ImGuizmo.h"

struct GpuBox {
    alignas(16) glm::mat4 transform;
    alignas(16) glm::mat4 invTransform;
    MaterialHandle materialHandle;
};

class Box: public Object {
public:
    Box(std::string name, glm::mat4 transform, MaterialHandle materialHandle);
    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;
    
    float getArea() override;
    GpuBox getStruct();
    glm::mat4 getTransform() const { return transform; }
    ObjectType getType() override { return ObjectType::Box; };

private:
    GpuBox box;

    glm::mat4 transform;
    MaterialHandle materialHandle;
};
