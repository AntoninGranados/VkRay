#pragma once

#include "core/render_structures.hpp"

struct DebugUBO {
    DebugView debugView = DebugView::None;
};

struct DisplayUBO {
    int showFocusPlane = 0;
    int selectedObjectId = -1;
    int previewBorderEnabled = 0;
    float lineWidth = 1.0f;
    alignas(8) glm::ivec2 viewportSize = {};
    alignas(8) glm::vec2 previewFrameExtent = glm::vec2(1.0f);
    alignas(16) glm::vec4 focusPlane = {};
    alignas(16) glm::mat4 inverseViewProjection = glm::mat4(1.0f);
    CameraUBO camera = {};
};

struct OverlayLine {
    glm::vec3 start;
    alignas(16) glm::vec3 end;
    alignas(16) glm::vec3 color;
    uint32_t onTop;
};

struct OverlayConstants {
    glm::mat4 viewProjection;
    glm::vec2 viewportSize;
    float lineWidth;
};

struct UnitVertex {
    glm::vec3 position;
    alignas(16) glm::vec3 normal;
};

struct RasterConstants {
    glm::mat4 viewProjection;
    glm::vec3 cameraPosition;
    uint32_t objectIndex;
    uint32_t firstIndex;
};
