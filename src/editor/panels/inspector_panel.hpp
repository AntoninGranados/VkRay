#pragma once

#include "core/ecs/entity.hpp"
#include "panel.hpp"

class Scene;

class InspectorPanel : public Panel {
public:
    std::string getTitle() const override { return "Inspector"; }
    void draw() override;

private:
    void drawAddComponentPopup(Scene& scene, ecs::Entity entity);
};
