#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "object/object.hpp"

const glm::vec3 up = { 0.0, 1.0, 0.0 };

class CameraHandle : public Object {
public:
    CameraHandle(std::string name, glm::vec3 position, glm::vec3 direction, float fov);

    glm::vec3 getDirection() const { return direction; };
    glm::vec3 getUp() const { return up; }
    glm::mat4 getView() const { return glm::lookAt(position, position + direction, up); };
    glm::mat4 getProjection(GLFWwindow* window) const;

    float rayIntersection(const Ray &ray) override;
    bool drawGuizmo(const glm::mat4 &view, const glm::mat4 &proj) override;
    bool drawUI(std::vector<Material> &materials) override;

    float getArea() override { return 0.0f; }
    ObjectType getType() override { return ObjectType::Camera; }

    float getFov() const { return fov; }
    void setFov(const float newFov) { fov = newFov; }

    glm::vec3 getPosition() const { return position; }
    void setPosition(const glm::vec3 newPosition) { position = newPosition; }

    void setDirection(const glm::vec3 newDirection) { direction = glm::normalize(newDirection); }

    void setSelected(bool isSelected) { selected = isSelected; }
    void setManipulationEnabled(bool enabled) { manipulationEnabled = enabled; }

    float getAperture() const { return aperture; };
    void setAperture(float newAperture) { aperture = newAperture; }
    
    float getFocusDepth() const { return focusDepth; };
    void setFocusDepth(float newFocusDepth) { focusDepth = newFocusDepth; }

protected:
    glm::vec3 position;
    glm::vec3 direction;

    float fov;

    float aperture = 0.0f;
    float focusDepth = 10.0f;
    float selectionRadius = 0.6f;

    void drawWireframe(const glm::mat4 &view, const glm::mat4 &proj);

    bool selected = false;
    bool manipulationEnabled = true;
};
