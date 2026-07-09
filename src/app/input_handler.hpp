#pragma once

struct AppContext;

class InputHandler {
public:
    void initCallbacks(const AppContext& ctx);
    void pollEvents(const AppContext& ctx);
    void handle(AppContext& ctx, float dt);

private:
    bool middleClickWasDown = false;
    bool spaceWasDown = false;

    void handlePreview(AppContext& ctx, float dt);
    void handleRender(AppContext& ctx, float dt);

    void returnToPreview(AppContext& ctx);
};
