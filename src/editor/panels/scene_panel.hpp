#pragma once

#include "panel.hpp"
#include "editor/scene/scene_ui.hpp"

class Scene;

class ScenePanel: IPanel {
public:
    ScenePanel(SceneSelection& selection) : selection(selection) {}
    void draw() override;

private:
    void drawNewObjectPopUp(Scene& scene);
    SceneSelection& selection;
};
