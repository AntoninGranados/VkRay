#include "object.hpp"

bool isInvalid(glm::mat4 mat) {
    bool invalid = false;
    for (size_t i = 0; i < 4; i++) {
        const glm::vec4 col = mat[i];
        invalid |= glm::any(glm::isnan(col)) || glm::any(glm::isinf(col));
    }
    return invalid;
}
