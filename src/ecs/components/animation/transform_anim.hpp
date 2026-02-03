#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ecs {

struct KeyVec3 { int frame; glm::vec3 value; };
struct KeyQuat { int frame; glm::quat value; };

struct TransformAnim {
    std::vector<KeyVec3> positionKeys;
    std::vector<KeyQuat> rotationKeys;
    std::vector<KeyVec3> scaleKeys;

    template<typename ValueT, typename KeyT>
    void insertKeyframe(int frame, const ValueT& data, std::vector<KeyT>& keyframes) {
        auto it = std::lower_bound(
            keyframes.begin(), keyframes.end(), frame,
            [](const KeyT& k, int f) -> bool { return k.frame < f; }
        );

        if (it != keyframes.end() && it->frame == frame) {
            *it = KeyT{ frame, data };
        } else {
            keyframes.insert(it, KeyT{ frame, data });
        }
    }

    void insertPositionKeyframe(int frame, const glm::vec3& pos) {
        insertKeyframe<glm::vec3, KeyVec3>(frame, pos, positionKeys);
    }
    bool hasPositionKeyframe(int frame) {
        auto it = std::find_if(
            positionKeys.begin(), positionKeys.end(),
            [frame](const KeyVec3& k) -> bool { return k.frame == frame; }
        );

        return it != positionKeys.end();
    }
    void insertRotationKeyframe(int frame, const glm::quat& rot) {
        insertKeyframe<glm::quat, KeyQuat>(frame, rot, rotationKeys);
    }
    void insertScaleframe(int frame, const glm::vec3& scale) {
        insertKeyframe<glm::vec3, KeyVec3>(frame, scale, scaleKeys);
    }
};

} // namespace ecs
