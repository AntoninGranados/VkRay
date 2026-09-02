#pragma once

#include <array>

#include "imgui/imgui.h"
#include "VkSmol/graph/builder_resource.hpp"

#include "panel.hpp"

class StatsPanel: public Panel {
private:
    void content() override;

    static constexpr int kHistorySize = 128;
    static constexpr int kNumPasses   = 5;

    struct PassInfo {
        const char*     name;
        ImU32           color;
        TimestampHandle timestamp;
    };

    struct FrameSample {
        std::array<float, kNumPasses> ms = {};
    };

    std::array<FrameSample, kHistorySize> history = {};
    int  historyHead  = 0;
    int  historyCount = 0;
    bool showGraph    = false;
};
