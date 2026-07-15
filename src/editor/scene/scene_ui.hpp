#pragma once

class Scene;

struct SceneSelection {
    int entity    = -1;
    int material  = -1;
    int meshAsset = -1;
};

class SceneUI {
public:
    void drawInspectors(Scene& scene, SceneSelection& selection);

private:
    void drawSelectedEntityUI(Scene& scene, SceneSelection& selection);
    void drawSelectedMaterialUI(Scene& scene, SceneSelection& selection);
    void drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection);
};
