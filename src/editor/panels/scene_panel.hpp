#pragma once

#include "panel.hpp"

class Scene;

class ScenePanel : public IPanel {
private:
    void content() override;
    void drawNewObjectPopUp(Scene& scene);
};
