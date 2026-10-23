#pragma once

#include <chrono>
#include <string>
#include <string_view>

struct ProgressStats {
    float progress;
    double elapsed;
    double eta;
    double rate;
};

class ProgressTimer {
public:
    void start();
    double elapsed() const;
    double eta(float progress) const;
    ProgressStats stats(uint32_t current, uint32_t total) const;

    static std::string formatTime(double seconds);

private:
    std::chrono::steady_clock::time_point t0;
};

class ProgressBar {
public:
    ProgressBar(std::string_view prefix, uint32_t total, std::string_view unit, int width = 40);
    void update(uint32_t current);
    void close();

private:
    void redraw();

    ProgressTimer timer;
    std::string prefix;
    uint32_t total;
    uint32_t current = 0;
    std::string unit;
    int width;
};
