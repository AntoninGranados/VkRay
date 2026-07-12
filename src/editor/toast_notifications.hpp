#pragma once

#include <chrono>
#include <vector>

#include "utils/log.hpp"

class ToastNotifications {
public:
    ToastNotifications();
    void draw();

private:
    struct Toast {
        LogEntry entry;
        float    timeLeft;
    };

    void push(const LogEntry& entry);

    std::vector<Toast>                    toasts;
    std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
};
