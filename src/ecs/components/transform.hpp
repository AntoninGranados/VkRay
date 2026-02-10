#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace ecs {

struct Transform {
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale    { 1.0f, 1.0f, 1.0f };

    glm::mat4 local { 1.0f };
    bool updated = true;

    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        updated = true;
    }
    
    void setRotation(const glm::quat& newRotation) {
        rotation = newRotation;
        updated = true;
    }
    
    void setScale(const glm::vec3& newScale) {
        scale = newScale;
        updated = true;
    }

    void updateLocal() {
        if (!updated) return;
        const glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 r = glm::toMat4(rotation);
        const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        local = t * r * s;
        updated = false;
    }
};

} // namespace ecs
