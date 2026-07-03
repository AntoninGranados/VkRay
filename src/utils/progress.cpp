#include "utils/progress.hpp"

#include <cstdio>
#include <iomanip>
#include <iostream>

// =========================== ProgressTimer ===========================

void ProgressTimer::start() {
    t0 = std::chrono::steady_clock::now();
}

double ProgressTimer::elapsed() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

double ProgressTimer::eta(float progress) const {
    if (progress <= 0.0f) return 0.0;
    const double e = elapsed();
    return e / static_cast<double>(progress) - e;
}

std::string ProgressTimer::formatTime(double seconds) {
    const int total = static_cast<int>(seconds);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    return buf;
}

// =========================== ProgressBar ===========================

ProgressBar::ProgressBar(std::string_view prefix, uint32_t total, std::string_view unit, int width)
    : prefix(prefix), total(total), unit(unit), width(width) {
    timer.start();
}

void ProgressBar::setPrefix(std::string_view p)  { prefix  = p; }
void ProgressBar::setPostfix(std::string_view p) { postfix = p; }

void ProgressBar::update(uint32_t n) {
    current = n;
    redraw();
}

void ProgressBar::step(uint32_t n) {
    current += n;
    redraw();
}

static void drawBlocks(int width, float progress) {
    const char* blocks[] = { "▏", "▎", "▍", "▌", "▋", "▊", "▉" };
    const float filledF = progress * static_cast<float>(width);
    const int   full    = static_cast<int>(filledF);
    const int   partial = static_cast<int>((filledF - static_cast<float>(full)) * 8.0f);

    for (int i = 0; i < full; i++) std::cout << "█";
    if (full < width) {
        std::cout << (partial > 0 ? blocks[partial - 1] : " ");
        for (int i = full + 1; i < width; i++) std::cout << ' ';
    }
}

void ProgressBar::close() {
    current = total;
    const double elapsed = timer.elapsed();
    const double rate    = elapsed > 0.0 ? static_cast<double>(total) / elapsed : 0.0;

    std::cout << '\r';
    if (!prefix.empty())  std::cout << prefix << ' ';
    
    std::cout << "100%";
    std::cout << '|'; drawBlocks(width, 1.0f); std::cout << "| ";

    std::cout << total << '/' << total << unit << ' ';

    std::cout << '[' << ProgressTimer::formatTime(elapsed) << ", "
              << std::fixed << std::setprecision(1) << rate << unit << "/s]";
    
    if (!postfix.empty()) std::cout << ' ' << postfix;
    std::cout << "\033[K\n";
}

void ProgressBar::redraw() {
    const float  progress = static_cast<float>(current) / static_cast<float>(total);
    const double elapsed  = timer.elapsed();
    const double rate     = elapsed > 0.0 ? static_cast<double>(current) / elapsed : 0.0;
    const int    digits   = static_cast<int>(std::to_string(total).size());

    std::cout << '\r';
    if (!prefix.empty())  std::cout << prefix << ' ';
    
    std::cout << std::setw(3) << static_cast<int>(progress * 100.0f) << '%';
    std::cout << '|'; drawBlocks(width, progress); std::cout << "| ";

    std::cout << std::setw(digits) << current << '/' << total << unit << ' ';

    std::cout << '[' << ProgressTimer::formatTime(elapsed)
              << '<' << ProgressTimer::formatTime(timer.eta(progress)) << ", "
              << std::fixed << std::setprecision(1) << rate << unit << "/s]";
    
    if (!postfix.empty()) std::cout << "  " << postfix;
    std::cout << "\033[K";
    std::cout.flush();
}
