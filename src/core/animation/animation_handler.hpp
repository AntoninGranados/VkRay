#pragma once

class AnimationHandler {
public:
    AnimationHandler(int endFrame, double fps);
    void setEndFrame(int newEndFrame) { endFrame = newEndFrame; }
    void setFps(double newFps) {
        fps = newFps;
        fixedDt = 1.0 / fps;
    }

    void reset(double t = 0.0);
    void reset(int frame = 0);
    void step(double dt);
    void stepFixed();

    int    getFrame()    const { return frame; }
    int    getEndFrame() const { return endFrame; }
    int    getFps()      const { return static_cast<int>(fps); }
    double getTime()     const { return time; }
    double getDt()       const { return dt; }
    double getFixedDt()  const { return fixedDt; }

    void pause()  { paused = true; }
    void play()   { paused = false; }
    void toggle() { paused = !paused; }
    bool isPaused() const { return paused; }

private:
    int endFrame;

    int frame;
    double time;

    double fps;
    double dt, fixedDt;

    bool paused;
};
