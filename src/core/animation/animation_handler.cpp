#include "animation_handler.hpp"

#include <cmath>

AnimationHandler::AnimationHandler(int endFrame, double fps)
    : endFrame(endFrame), frame(0), time(0.0), fps(fps), dt(0.0), fixedDt(1.0 / fps), paused(true) {}

void AnimationHandler::reset(double t) {
    time = t;
    frame = static_cast<int>(std::floor(time * fps));
}

void AnimationHandler::reset(int _frame) {
    frame = _frame;
    time = static_cast<double>(_frame) / fps;
}

void AnimationHandler::step(double _dt) {
    dt = _dt;
    const int newFrame = static_cast<int>(std::floor((time + _dt) * fps));
    if (newFrame >= endFrame) {
        time = _dt;
        frame = 0;
    } else {
        time += _dt;
        frame = newFrame;
    }
}

void AnimationHandler::stepFixed() {
    dt = fixedDt;
    const int newFrame = frame + 1;
    if (newFrame >= endFrame) {
        time = fixedDt;
        frame = 0;
    } else {
        time += fixedDt;
        frame = newFrame;
    }
}
