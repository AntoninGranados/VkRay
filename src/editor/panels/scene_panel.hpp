#pragma once

#include "panel.hpp"
#include "editor/scene/scene_ui.hpp"

class Scene;

class ScenePanel: public IPanel {
public:
    ScenePanel(SceneSelection& selection) : selection(selection) {}
    
private:
    void content() override;
    void drawNewObjectPopUp(Scene& scene);
    SceneSelection& selection;
};
