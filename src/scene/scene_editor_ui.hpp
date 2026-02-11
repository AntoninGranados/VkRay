#pragma once

#include <glm/glm.hpp>

class Scene;

class SceneEditorUI {
public:
    static void drawGuizmo(Scene& scene, const glm::mat4& view, const glm::mat4& proj);
    static void drawUI(Scene& scene);
    static void drawNewObjectPopUp(Scene& scene);
    static void drawSelectedEntityUI(Scene& scene);
    static void drawSelectedMaterialUI(Scene& scene);
    static void drawSelectedMeshAssetUI(Scene& scene);
};
