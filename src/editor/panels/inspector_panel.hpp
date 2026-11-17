#pragma once

#include <string>

#include "core/ecs/entity.hpp"
#include "panel.hpp"

class Scene;

class InspectorPanel : public Panel {
public:
    std::string getTitle() const override { return "Inspector"; }
    DockSlot getDockSlot() const override { return DockSlot::Right; }
    void draw() override;

private:
    void drawAddComponentPopup(Scene& scene, ecs::Entity entity);

    std::string componentSearch;
};
