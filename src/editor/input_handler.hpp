#pragma once

class InputHandler {
public:
    void initCallbacks();
    void pollEvents();
    void handle(float dt);

private:
    bool spaceWasDown = false;
    bool leftWasDown = false;
    bool rightWasDown = false;
    float leftRepeat = 0.0f;
    float rightRepeat = 0.0f;

    void handlePreview(float dt);
    void handleRender(float dt);

    void returnToPreview();
};
