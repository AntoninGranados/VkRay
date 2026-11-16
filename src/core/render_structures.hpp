#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>

static constexpr int kMediumStackSize = 16;

enum class RenderMode { Preview, RenderSingle, RenderAnimation };

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
    Uv,
};

enum class ViewportShading : int { Rasterized = 0, PathTraced };

struct alignas(16) ThinLensUBO {
    float lensRadius;
    float focusDistance;
};

struct alignas(16) CameraUBO {
    float U;
    float V;
    ThinLensUBO thinLens;
    uint32_t motionOffset;
    int32_t lensSlot = -1;
    int32_t projection = 0;
    int32_t lensParamsBase = 0;
};

struct alignas(16) ScreenUBO {
    alignas(8) glm::vec2 size;
    float aspect;
};

struct alignas(16) RenderUBO {
    int32_t skySlot = 0;
    int32_t skyParamsBase = 0;
    int maxBounces;
    int importanceSampling;
    int clipAccumulation;
    float clipThreshold;
    int varianceSampling;
    int varianceWarmupSamples;
    int cameraMediumCount = 0;
    alignas(16) std::array<int32_t, kMediumStackSize> cameraMedia{};
};

struct PathtracerUBO {
    uint32_t sampleCount;
    CameraUBO camera;
    ScreenUBO screen;
    RenderUBO render;
};

struct alignas(16) CompositingPassUBO {
    int32_t slot = 0;
    int32_t paramsBase = 0;
    int32_t passId = 0;
};

inline constexpr const char* kAOVNames[] = {"position_w", "position",  "normal_w", "normal",
                                            "albedo",     "roughness", "mat_type", "sky_mask"};

struct alignas(16) AOVBuffer {
    alignas(16) uint32_t hitValid;
    alignas(16) glm::vec3 positionW;
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 normalW;
    alignas(8) glm::vec2 normal;
    alignas(16) glm::vec3 albedo;
    float roughness;
    uint32_t matType;
    uint32_t skyMask;
    alignas(8) glm::vec2 uv;
};

struct PixelInfo {
    AOVBuffer aov;
    float mean;
    float m2;
    int count;
    uint32_t bounces;
    uint32_t bvhChecks;
    uint32_t triangleChecks;
    float varianceProba;
};
