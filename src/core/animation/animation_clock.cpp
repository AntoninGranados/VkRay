#include "animation_clock.hpp"

#include <cmath>

AnimationClock::AnimationClock(int endFrame, double fps)
    : endFrame(endFrame), fps(fps), fixedDt(1.0 / fps) {}

void AnimationClock::reset(double t) {
    time = t;
    frame = static_cast<int>(std::floor(time * fps));
}

void AnimationClock::reset(int _frame) {
    frame = _frame;
    time = static_cast<double>(_frame) / fps;
}

void AnimationClock::step(double _dt) {
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

void AnimationClock::stepFixed() {
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

bool AnimationClock::sample(float jitterRange) {
    frameChanged = frame != sampledFrame;
    sampledFrame = frame;

    if (frameChanged || jitterRange <= 0.0f) {
        sampleFrame = static_cast<float>(frame);
    } else {
        std::uniform_real_distribution<float> dist(-jitterRange * 0.5f, jitterRange * 0.5f);
        sampleFrame = static_cast<float>(frame) + dist(rng);
    }

    return frameChanged;
}
