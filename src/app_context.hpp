#pragma once

#include "./engine/engine.hpp"
#include "./scene/scene.hpp"
#include "./camera.hpp"
#include "./parameters.hpp"
#include "./notification.hpp"

struct RendererState {
    bool renderMode = false;
    bool pendingExit = false;
    double samplesPerSecEMA = 0.0;
    bool samplesPerSecInitialized = false;
    double samplesPerSecAccumTime = 0.0;
    double samplesPerSecAccumSamples = 0.0;

    uint32_t sampleCount = 0;
    float resolution = 1.0f;
    float prevResolution = 1.0f;
};

struct PathtracerUBO {
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::vec3 cameraDir;
    float tanHFov;
    float aperture;
    float focusDepth;

    alignas(8) glm::vec2 screenSize;
    float aspect;
    float resolution;
    float prevResolution;
    
    int frameCount;
    float time;

    int lightMode;

    int maxBounces;
    int samplesPerPixel;
    int importanceSampling;
    int varianceSampling;
    int varianceWarmupSamples;
    int debugView;
};

struct ScreenUBO {
    int frameCount;
    float resolution;
    int debugView;
    int previewBorderEnabled;
};


struct AppContext {
    VkSmol* engine;
    Scene* scene;
    Camera* camera;
    ParameterStore* parameters;
    NotificationManager* notifications;

    RendererState* renderer;

    PathtracerUBO* pathtracerUBO;
    ScreenUBO* screenUBO;

    bool* restartRender = nullptr;
};
