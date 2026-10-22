#pragma once

#include <array>
#include <string>

#include "imgui/imgui.h"

#include "panel.hpp"

class StatsPanel : public Panel {
public:
    std::string getTitle() const override { return "FPS"; }
    void draw() override;

private:
    static constexpr int kHistorySize = 128;
    static constexpr int kNumPasses = 5;

    struct PassInfo {
        const char* name;
        ImU32 color;
        std::string passName;
    };

    struct FrameSample {
        std::array<float, kNumPasses> ms = {};
    };

    std::array<FrameSample, kHistorySize> history = {};
    int historyHead = 0;
    int historyCount = 0;
    bool showGraph = false;
};
