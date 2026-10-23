#pragma once

#include <memory>
#include <vector>

#include "editor/panels/panel.hpp"
#include "editor/panels/viewport_panel.hpp"

class EditorUi {
public:
    EditorUi();
    void draw();

    bool isMouseCaptured() { return capturesMouse; }
    bool isKeyboardCaptured() { return capturesKeyboard; }

    ImVec2 getViewportSize() const { return viewportPanel->getSize(); }
    ImVec2 getViewportPos() const { return viewportPanel->getPos(); }
    ImDrawList* getViewportDrawList() const { return viewportPanel->getDrawList(); }

private:
    std::vector<std::unique_ptr<Panel>> previewPanels;
    std::vector<std::unique_ptr<Panel>> renderPanels;
    ViewportPanel* viewportPanel = nullptr;

    bool capturesMouse = false;
    bool capturesKeyboard = false;

    template <typename T> T& addPanel(std::vector<std::unique_ptr<Panel>>& panels) {
        return static_cast<T&>(*panels.emplace_back(std::make_unique<T>()));
    }

    static void initStyle();
    void setupDockspace();
    void updateState();
};
