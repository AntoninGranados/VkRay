#pragma once

#include <glm/glm.hpp>

class Scene;

struct SceneSelection {
    int entity    = -1;
    int material  = -1;
    int meshAsset = -1;
};

class SceneUI {
public:
    void drawGuizmo(Scene& scene, SceneSelection& selection, const glm::mat4& view, const glm::mat4& proj);
    void drawInspectors(Scene& scene, SceneSelection& selection);
    int  raycast(Scene& scene, const glm::vec2& screenPos, const glm::vec2& screenSize, float& dist, bool includeCameras = true);

private:
    void drawSelectedEntityUI(Scene& scene, SceneSelection& selection);
    void drawSelectedMaterialUI(Scene& scene, SceneSelection& selection);
    void drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection);
};
