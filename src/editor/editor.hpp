#pragma once

#include <optional>
#include <string>

#include "VkSmol/ui/imgui_layer.hpp"

#include "core/core.hpp"
#include "core/ecs/entity.hpp"
#include "editor_renderer.hpp"
#include "editor_ui.hpp"
#include "input_handler.hpp"
#include "material_preview.hpp"

class Editor {
public:
    static void init();
    static void run();

    static VkSmol::ImGuiLayer& getImGui() { return get().imgui; }
    static EditorUi& getUi();
    static EditorRenderer& getEditorRenderer();
    static MaterialPreview& getMaterialPreview();

    static std::optional<ecs::Entity> getSelectedEntity();
    static void selectEntity(std::optional<ecs::Entity> entity);

    static bool loadScene(const std::string& path);

    static ecs::Entity getViewportCamera() { return get().viewportCamera; }
    static bool isUsingSceneCamera();
    static void activateSceneCamera();

private:
    Editor() = default;
    static Editor& get();

    static void setSelectedEntity(ecs::Entity entity);
    static void clearSelectedEntity();

    static void createViewportCamera();

    static void stepAnimation(float deltaTime);
    static void handleViewportResize();
    static void handleRenderModeCompletion();
    static void updateFrustumScale();

    VkSmol::ImGuiLayer imgui{Core::getEngine(), Core::getWindow()};
    EditorUi ui;
    InputHandler inputHandler;
    EditorRenderer editorRenderer;
    MaterialPreview materialPreview;

    std::optional<ecs::Entity> selectedEntity;
    ecs::Entity viewportCamera;
};
