#pragma once

#include "panel.hpp"
#include "app_context.hpp"
#include "editor/scene/scene_ui.hpp"

class Scene;

class ScenePanel: IPanel {
public:
    ScenePanel(SceneSelection& selection) : selection(selection) {}
    void draw(AppContext& ctx) override;

private:
    void drawNewObjectPopUp(Scene& scene);
    SceneSelection& selection;
};
