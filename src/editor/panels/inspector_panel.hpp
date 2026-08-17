#pragma once

#include "core/ecs/entity.hpp"
#include "panel.hpp"

class Scene;

class InspectorPanel : public IPanel {
private:
    void content() override;
    void drawAddComponentPopup(Scene& scene, ecs::Entity entity);
};
