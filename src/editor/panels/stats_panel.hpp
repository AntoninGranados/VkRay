#pragma once

#include <array>

#include "panel.hpp"

class StatsPanel: public IPanel {
private:
    void content() override;

    static constexpr int kHistorySize = 128;
    static constexpr int kNumPasses   = 5;

    struct FrameSample {
        std::array<float, kNumPasses> ms = {};
    };

    std::array<FrameSample, kHistorySize> history = {};
    int  historyHead  = 0;
    int  historyCount = 0;
    bool showGraph    = false;
};
