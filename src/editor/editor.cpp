#include "editor.hpp"

Editor& Editor::get() {
    static Editor instance;
    return instance;
}

void           Editor::init()            { get().inputHandler.initCallbacks(); }
EditorUi&      Editor::getUi()           { return get().ui; }
InputHandler&  Editor::getInputHandler() { return get().inputHandler; }
EditorRenderer& Editor::getEditorRenderer() { return get().editorRenderer; }
