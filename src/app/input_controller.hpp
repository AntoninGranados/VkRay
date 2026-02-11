#pragma once

#include <GLFW/glfw3.h>

struct AppContext;

class InputController {
public:
    void initCallbacks(const AppContext& ctx);
    void pollEvents();
    void handle(const AppContext& ctx, float dt);

private:
    bool middleClickWasDown = false;
    bool spaceWasDown = false;

    void handlePreview(const AppContext& ctx, float dt);
    void handleRender(const AppContext& ctx, float dt);

    void updateRenderSamplesPerSecond(const AppContext& ctx, float dt);
    void returnToPreview(const AppContext& ctx);
};
