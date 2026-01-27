#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace ecs {

struct Transform {
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    bool positionToggled = true;
    glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    bool rotationToggled = true;
    glm::vec3 scale    { 1.0f, 1.0f, 1.0f };
    bool scaleToggled = true;

    glm::mat4 local { 1.0f };
    bool updated = true;

    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        updated = true;
    }
    void setPositionToggle(const bool& toggled) { positionToggled = toggled; }
    
    void setRotation(const glm::quat& newRotation) {
        rotation = newRotation;
        updated = true;
    }
    void setRotationToggle(const bool& toggled) { rotationToggled = toggled; }
    
    void setScale(const glm::vec3& newScale) {
        scale = newScale;
        updated = true;
    }
    void setScaleToggle(const bool& toggled) { scaleToggled = toggled; }

    void updateLocal() {
        if (!updated) return;
        const glm::mat4 t = positionToggled ? glm::translate(glm::mat4(1.0f), position) : glm::mat4(1.0f);
        const glm::mat4 r = rotationToggled ? glm::toMat4(rotation) : glm::mat4(1.0f);
        const glm::mat4 s = scaleToggled ? glm::scale(glm::mat4(1.0f), scale) : glm::mat4(1.0f);
        local = t * r * s;
        updated = false;
    }
};

} // namespace ecs
