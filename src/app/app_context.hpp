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

struct DisplayUBO {
    int frameCount;
    float resolution;
    int debugView;
    int previewBorderEnabled;
    int denoisingEnabled;
};

struct PixelInfo {
    alignas(16) glm::vec4 normal;
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 diffuse;
    float mean;
    float m2;
    int count;
    float varianceProba;
    int selectionMask;
};

class VkSmol;
class Scene;
class Camera;
class ParameterHandler;
class NotificationHandler;
class EditorUi;
class AnimationHandler;
class Platform;

struct AppContext {
    VkSmol* engine;
    Scene* scene;
    Camera* camera;
    ParameterHandler* parameters;
    NotificationHandler* notifications;
    EditorUi* ui;
    AnimationHandler* animation;

    RenderState* renderState;
    PathtracerUBO* pathtracerUBO;
    DisplayUBO* displayUBO;

    bool* restartRender = nullptr;
    Platform* platform = nullptr;
};
