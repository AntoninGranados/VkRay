#pragma once

class AnimationClock {
public:
    AnimationClock(int endFrame, double fps);
    void setEndFrame(int newEndFrame) { endFrame = newEndFrame; }
    void setFps(double newFps) {
        fps = newFps;
        fixedDt = 1.0 / fps;
    }

    void reset(double t = 0.0);
    void reset(int frame = 0);
    void step(double dt);
    void stepFixed();

    bool sample();
    bool didFrameChange() const { return frameChanged; }

    int    getFrame()       const { return frame; }
    int    getEndFrame()    const { return endFrame; }
    int    getFps()         const { return static_cast<int>(fps); }
    double getTime()        const { return time; }
    double getDt()          const { return dt; }
    double getFixedDt()     const { return fixedDt; }

    void pause()  { paused = true; }
    void play()   { paused = false; }
    void toggle() { paused = !paused; }
    bool isPaused() const { return paused; }

private:
    int endFrame;

    int frame = 0;
    int sampledFrame = -1;
    bool frameChanged = false;
    double time = 0.0;

    double fps;
    double dt = 0.0, fixedDt;

    bool paused = true;
};
