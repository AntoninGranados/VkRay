#pragma once

#include <glm/glm.hpp>

enum class RenderMode {
    Preview,
    RenderSingle,
    RenderAnimation
};

struct RenderState {
    RenderMode renderMode = RenderMode::Preview;
    bool pendingExit = false;
    double samplesPerSecEMA = 0.0;
    bool samplesPerSecInitialized = false;
    double samplesPerSecAccumTime = 0.0;
    double samplesPerSecAccumSamples = 0.0;

    uint32_t sampleCount = 0;
    float resolution = 1.0f;
    float prevResolution = 1.0f;
};

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

struct DisplayUBO {
    float resolution;
    int   debugView;
    int   previewBorderEnabled;
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
