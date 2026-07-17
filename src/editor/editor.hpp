#pragma once

#include "editor_ui.hpp"
#include "editor_renderer.hpp"
#include "input_handler.hpp"

class Editor {
public:
    static void init();
    static void run();

    static EditorUi&       getUi();
    static InputHandler&   getInputHandler();
    static EditorRenderer& getEditorRenderer();

private:
    Editor() = default;
    static Editor& get();

    EditorUi       ui;
    InputHandler   inputHandler;
    EditorRenderer editorRenderer;
};
