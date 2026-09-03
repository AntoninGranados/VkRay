#pragma once

#include <optional>

#include "core/core.hpp"
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
    static EditorRenderer& getEditorRenderer();
    static MaterialPreview& getMaterialPreview();

    static ImVec2      getViewportPos()      { return getUi().getViewportPos(); }
    static ImVec2      getViewportSize()     { return getUi().getViewportSize(); }
    static ImDrawList* getViewportDrawList() { return getUi().getViewportDrawList(); }

    static std::optional<ecs::Entity> getSelectedEntity();
    static void selectEntity(std::optional<ecs::Entity> entity);

private:
    Editor() = default;
    static Editor& get();

    static void setSelectedEntity(ecs::Entity entity);
    static void clearSelectedEntity();

    static void stepAnimation(float deltaTime);
    static void handleViewportResize();
    static void handleRenderModeCompletion();

    EditorUi ui;
    InputHandler inputHandler;
    EditorRenderer editorRenderer;
    MaterialPreview materialPreview;

    std::optional<ecs::Entity> selectedEntity;
};
