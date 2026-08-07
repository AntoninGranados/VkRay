#pragma once

#include <unordered_map>

class InputHandler {
public:
    void initCallbacks();
    void pollEvents();
    void handle(float dt);

private:
    std::unordered_map<int, bool> prevKeys;
    float leftRepeat  = 0.0f;
    float rightRepeat = 0.0f;

    bool justPressed(int key);

    void handlePreview(float dt);
    void handleRender(float dt);

    void returnToPreview();
};
