#pragma once

#include "./app_context.hpp"
#include "./engine/engine.hpp"

#include "./imgui/imgui.h"

class UiHandler {
public:
    void draw(CommandBuffer commandBuffer, AppContext& ctx);

    bool isToggled() { return toggled; }
    void toggle() { toggled = !toggled; }
    void setToggle(bool newToggle) { toggled = newToggle; }
    void saveToggledState() { toggleState = toggled; }
    void restorToggledState() { toggled = toggleState; }

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

private:
    bool toggled = true;
    bool toggleState = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;

    void updateState();

    void drawPreview(AppContext& ctx);
    void drawRender(AppContext& ctx);
};
