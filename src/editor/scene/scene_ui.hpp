#pragma once

#include <optional>

#include "core/ecs/entity.hpp"

class Scene;

struct SceneSelection {
    std::optional<ecs::Entity> entity;
    int meshAsset = -1;
};

class SceneUI {
public:
    void drawInspectors(Scene& scene, SceneSelection& selection);

private:
    void drawSelectedEntityUI(Scene& scene, SceneSelection& selection);
    void drawSelectedMeshAssetUI(Scene& scene, SceneSelection& selection);
};
