#pragma once

class InputHandler {
public:
    void initCallbacks();
    void pollEvents();
    void handle(float dt);

private:
    bool spaceWasDown = false;

    void handlePreview(float dt);
    void handleRender(float dt);

    void returnToPreview();
};
