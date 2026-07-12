#pragma once

#include <glm/glm.hpp>

// ===================== Uniform Buffers =====================
struct alignas(16) CameraUBO {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    float tanHFov;
    float aperture;
    float focusDepth;
};

struct alignas(16) ScreenUBO {
    alignas(8) glm::vec2 size;
    float aspect;
};

struct alignas(16) RenderUBO {
    int lightMode;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipValue;
    int varianceSampling;
    int varianceWarmupSamples;
    int debugView;
};

struct PathtracerUBO {
    uint32_t  sampleCount;
    int32_t   selectedObjectId = -1;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
};

struct CompositingUBO {
    int denoisingEnabled;
};

// ===================== Arbitrary Outputs Variables =====================
struct AOVFlags {
    bool positionW = false;
    bool position  = false;
    bool normalW   = false;
    bool normal    = false;
    bool albedo    = false;
    bool roughness = false;
    bool matType   = false;
    bool skyMask   = false;
};

struct alignas(16) AOVBuffer {
    alignas(16) uint32_t  hitValid;
    alignas(16) glm::vec3 positionW;
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normalW;
    alignas(8)  glm::vec2 normal;
    alignas(16) glm::vec3 albedo;
    float                 roughness;
    uint32_t              matType;
    uint32_t              skyMask;
};

struct PixelInfo {
    AOVBuffer aov;
    float    mean;
    float    m2;
    int      count;
    float    varianceProba;
    uint32_t selectionMask;
};

