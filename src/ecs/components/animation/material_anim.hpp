#pragma once

#include <algorithm>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// ! Only supported for Principled BSDF
namespace ecs {

struct KeyMaterial {
    int frame;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    float emissionStrength;
};

struct MaterialAnim {
    std::vector<KeyMaterial> materialsKeys;

    bool hasKeyframe(int frame) {
        const auto it = std::lower_bound(
            materialsKeys.begin(), materialsKeys.end(), frame,
            [](const KeyMaterial& k, int f) -> bool { return k.frame < f; }
        );

        return it != materialsKeys.end() && it->frame == frame;
    }

    void insertKeyframe(int frame, float roughness, float metalness, float ior, float transmission, float emissionStrength) {
        auto it = std::lower_bound(
            materialsKeys.begin(), materialsKeys.end(), frame,
            [](const KeyMaterial& k, int f) -> bool { return k.frame < f; }
        );

        if (it != materialsKeys.end() && it->frame == frame) {
            *it = KeyMaterial{ frame, roughness, metalness, ior, transmission, emissionStrength };
        } else {
            materialsKeys.insert(it, KeyMaterial{ frame, roughness, metalness, ior, transmission, emissionStrength });
        }
    }

    void removeKeyframe(int frame) {
        auto it = std::lower_bound(
            materialsKeys.begin(), materialsKeys.end(), frame,
            [](const KeyMaterial& k, int f) -> bool { return k.frame < f; }
        );

        if (it != materialsKeys.end() && it->frame == frame) {
            materialsKeys.erase(it);
        }
    }

    KeyMaterial getKeyframe(int frame) const {
        if (materialsKeys.empty()) return { frame, 0.0f, 0.0f, 1.5f, 0.0f, 0.0f };
        if (frame <= materialsKeys.front().frame) return materialsKeys.front();
        if (frame >= materialsKeys.back().frame)  return materialsKeys.back();

        auto next = std::lower_bound(materialsKeys.begin(), materialsKeys.end(), frame,
            [](const KeyMaterial& k, int f) -> bool { return k.frame < f; });
        auto prev = std::prev(next);

        const float t = static_cast<float>(frame - prev->frame) /
                        static_cast<float>(next->frame - prev->frame);
        return KeyMaterial{
            frame,
            prev->roughness        + t * (next->roughness        - prev->roughness),
            prev->metalness        + t * (next->metalness        - prev->metalness),
            prev->ior              + t * (next->ior              - prev->ior),
            prev->transmission     + t * (next->transmission     - prev->transmission),
            prev->emissionStrength + t * (next->emissionStrength - prev->emissionStrength),
        };
    }
};

} // namespace ecs
