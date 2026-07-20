#pragma once

#include "core/structures.hpp"

struct DisplayUBO {
    int previewBorderEnabled = 0;
};

struct DebugUBO {
    DebugView debugView = DebugView::None;
};
