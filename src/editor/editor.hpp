#pragma once

#include <optional>

#include "core/ecs/entity.hpp"
#include "editor_ui.hpp"
#include "editor_renderer.hpp"
#include "input_handler.hpp"
#include "material_preview.hpp"

class Editor {
public:
    static void init();
    static void run();

    static EditorUi& getUi();
    static InputHandler& getInputHandler();
    static EditorRenderer& getEditorRenderer();
    static MaterialPreview& getMaterialPreview();

    static std::optional<ecs::Entity> getSelectedEntity();
    static void setSelectedEntity(ecs::Entity entity);
    static void clearSelectedEntity();

private:
    Editor() = default;
    static Editor& get();

    static void stepAnimation(float deltaTime);
    static void handleViewportResize();
    static void handleRenderModeCompletion();

    EditorUi ui;
    InputHandler inputHandler;
    EditorRenderer editorRenderer;
    MaterialPreview materialPreview;
    std::optional<ecs::Entity> selectedEntity;
};
