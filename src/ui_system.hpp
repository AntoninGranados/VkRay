#pragma once

#include "./app_context.hpp"
#include "./engine/engine.hpp"
#include "./parameter_system.hpp"
#include "./scene/scene_preset.hpp"

#include "./imgui/imgui.h"

class UiSystem {
public:
    void draw(CommandBuffer commandBuffer, AppContext& ctx);

    bool isToggled() { return toggled; }
    void toggle() { toggled = !toggled; }
    void setToggle(bool newToggle) { toggled = newToggle; }
    void saveToggledState() { toggleState = toggled; }
    void restorToggledState() { toggled = toggleState; }

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

    bool wasMiddleClickDown() { return middleClickWasDown; }
    void setMiddleClickState(bool middleClickDown) { middleClickWasDown = middleClickDown; }

private:
    bool toggled = true;
    bool toggleState = true;
    bool capturesMouse = false;
    bool capturesKeyboard = false;
    bool middleClickWasDown = false;

    void updateState();

    void drawPreview(AppContext& ctx);
    void drawRender(AppContext& ctx);
};
