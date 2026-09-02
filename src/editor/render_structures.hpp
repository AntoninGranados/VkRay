#pragma once

#include "core/render_structures.hpp"

struct DebugUBO {
    DebugView debugView = DebugView::None;
};

struct DisplayUBO {
    int showFocusPlane = 0;
    int selectedObjectId = -1;
    int previewBorderEnabled = 0;
    alignas(16) glm::vec4 focusPlane = {};
    CameraUBO camera = {};
};
