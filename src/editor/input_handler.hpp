#pragma once

#include <unordered_map>

class InputHandler {
public:
    void initCallbacks();
    void pollEvents();
    void handle(float dt);

private:
    static constexpr float kFrameStepRepeatDelay    = 0.4f;
    static constexpr float kFrameStepRepeatInterval = 0.05f;

    std::unordered_map<int, bool> prevKeys;
    std::unordered_map<int, float> repeatTimers;

    bool justPressed(int key);
    static bool isMouseInputBlocked();

    void handlePreview(float dt);
    void handleRender(float dt);
    void handleFrameStepKey(int key, int direction, float dt, bool blocked);

    void returnToPreview();
};
