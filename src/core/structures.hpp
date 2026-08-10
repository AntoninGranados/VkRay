#pragma once

#include <glm/glm.hpp>

enum class RenderMode {
    Preview,
    RenderSingle,
    RenderAnimation
};

enum class DebugView : int {
    None = 0,
    PositionW,
    Position,
    NormalW,
    Normal,
    Albedo,
    Roughness,
    MatType,
    Bounces,
    HitChecks,
    Variance,
    SkyMask,
};

enum LightMode : int {
    Day,
    Sunset,
    Night,
    Empty,
};

struct alignas(16) ThinLensUBO {
    float lensRadius;
    float focusDistance;
};

struct alignas(16) TiltShiftUBO {
    alignas(16) glm::vec3 focusA;
    alignas(16) glm::vec3 focusB;
    alignas(16) glm::vec3 focusC;
    int enabled;
};

struct alignas(16) CameraUBO {
    alignas(16) glm::vec3 eye;
    alignas(16) glm::vec3 U;
    alignas(16) glm::vec3 V;
    alignas(16) glm::vec3 W;
    ThinLensUBO thinLens;
    TiltShiftUBO tiltShift;
};

struct alignas(16) ScreenUBO {
    alignas(8) glm::vec2 size;
    float aspect;
};

struct alignas(16) RenderUBO {
    LightMode lightMode;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipThreshold;
    int varianceSampling;
    int varianceWarmupSamples;
};

struct PathtracerUBO {
    uint32_t  sampleCount;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
};

struct CompositingUBO {
    int denoisingEnabled;
};

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
    uint32_t bounces;
    uint32_t bvhChecks;
    uint32_t triangleChecks;
    float    varianceProba;
};

