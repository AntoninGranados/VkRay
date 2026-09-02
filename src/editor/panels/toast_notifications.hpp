#pragma once

#include <chrono>
#include <vector>

#include "panel.hpp"
#include "utils/log.hpp"

class ToastNotifications : public Panel {
public:
    ToastNotifications();

private:
    struct Toast {
        LogEntry entry;
        float    timeLeft;
    };

    void push(const LogEntry& entry);
    void content() override;

    std::vector<Toast>                    toasts;
    std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
};
