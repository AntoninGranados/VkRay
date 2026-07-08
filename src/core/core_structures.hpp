#pragma once

#include <glm/glm.hpp>

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
    float resolution;
    float prevResolution;
};

struct alignas(16) FrameUBO {
    int count;
    float time;
};

struct alignas(16) RenderUBO {
    int lightMode;
    int maxBounces;
    int samplesPerPixel;
    int importanceSampling;
    int varianceSampling;
    int varianceWarmupSamples;
    int clipAccumulation;
    int debugView;
};

struct PathtracerUBO {
    CameraUBO camera;
    ScreenUBO screen;
    FrameUBO frame;
    RenderUBO render;
};

struct CompositingUBO {
    float resolution;
    int   denoisingEnabled;
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

