#include "app/animation_handler.hpp"

#include <cmath>

AnimationHandler::AnimationHandler(int endFrame, double fps):
endFrame(endFrame), fps(fps) {
    paused = true;
    fixedDt = 1.0 / fps;
    reset(0);
}

void AnimationHandler::reset(double t) {
    time = t;
    frame = static_cast<int>(std::floor(time * fps));
}

void AnimationHandler::reset(int _frame) {
    frame = _frame;
    time = static_cast<float>(frame) / fps;
}

void AnimationHandler::step(double _dt) {
    dt = _dt;
    
    int newFrame = static_cast<int>(std::floor((time + _dt) * fps));
    if (newFrame >= endFrame) {
        time = dt;
        frame = 0;
    } else {
        time += dt;
        frame = newFrame;
    }
}

void AnimationHandler::stepFixed() {    
    dt = fixedDt;

    int newFrame = frame + 1;
    if (newFrame >= endFrame) {
        time = dt;
        frame = 0;
    } else {
        time += dt;
        frame = newFrame;
    }
}
