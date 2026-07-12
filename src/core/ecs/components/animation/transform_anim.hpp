#pragma once

#include <algorithm>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ecs {

struct KeyVec3 { int frame; glm::vec3 value; };
struct KeyQuat { int frame; glm::quat value; };

enum class TransformKeyframeType {
    Position,
    Rotation,
    Scale
};

struct TransformAnim {
    std::vector<KeyVec3> positionKeys;
    std::vector<KeyQuat> rotationKeys;
    std::vector<KeyVec3> scaleKeys;

    template<typename KeyT>
    bool hasKeyframe(int frame, const std::vector<KeyT>& keyframes) {
        const auto it = std::lower_bound(
            keyframes.begin(), keyframes.end(), frame,
            [](const KeyT& k, int f) -> bool { return k.frame < f; }
        );

        return it != keyframes.end() && it->frame == frame;
    }

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

    template<typename KeyT>
    void removeKeyframe(int frame, std::vector<KeyT>& keyframes) {
        auto it = std::lower_bound(
            keyframes.begin(), keyframes.end(), frame,
            [](const KeyT& k, int f) -> bool { return k.frame < f; }
        );

        if (it != keyframes.end() && it->frame == frame) {
            keyframes.erase(it);
        }
    }

    void insertPositionKeyframe(int frame, const glm::vec3& pos) {
        insertKeyframe(frame, pos, positionKeys);
    }
    void removePositionKeyframe(int frame) {
        removeKeyframe(frame, positionKeys);
    }
    bool hasPositionKeyframe(int frame) {
        return hasKeyframe(frame, positionKeys);
    }

    void insertRotationKeyframe(int frame, const glm::quat& rot) {
        insertKeyframe(frame, rot, rotationKeys);
    }
    void removeRotationKeyframe(int frame) {
        removeKeyframe(frame, rotationKeys);
    }
    bool hasRotationKeyframe(int frame) {
        return hasKeyframe(frame, rotationKeys);
    }

    void insertScaleKeyframe(int frame, const glm::vec3& scale) {
        insertKeyframe(frame, scale, scaleKeys);
    }
    void removeScaleKeyframe(int frame) {
        removeKeyframe(frame, scaleKeys);
    }
    bool hasScaleKeyframe(int frame) {
        return hasKeyframe(frame, scaleKeys);
    }
};

} // namespace ecs
