#include "transform_utils.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

glm::mat4 composeTransform(const ecs::Component& transform) {
    return glm::translate(glm::mat4(1.0f), transform.get<glm::vec3>("position")) *
           glm::mat4_cast(glm::quat(glm::radians(transform.get<glm::vec3>("rotation")))) *
           glm::scale(glm::mat4(1.0f), transform.get<glm::vec3>("scale"));
}

void applyTransform(ecs::Component& transform, const glm::mat4& matrix) {
    glm::vec3 translation, scale, skew;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);

    const glm::quat oldRotation = glm::quat(glm::radians(transform.get<glm::vec3>("rotation")));

    transform.set<glm::vec3>("position", translation);
    transform.set<glm::vec3>("scale", scale);
    if (glm::abs(glm::dot(oldRotation, rotation)) < 0.99999f)
        transform.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(rotation)));
}
