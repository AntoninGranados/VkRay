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

struct DisplayUBO {
    float resolution;
    int   debugView;
    int   previewBorderEnabled;
};
