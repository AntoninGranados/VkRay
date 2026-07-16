#pragma once

#include <functional>

#include <glm/glm.hpp>
#include "imgui/imgui.h"

#include "editor/panels/panel.hpp"

class Scene;
struct SceneSelection;

class ViewportPanel: public IPanel {
public:
void setOnEntitySelectionCallback(std::function<void(int)> callback) { onEntitySelection = callback; }

ImVec2      getSize()     const { return size; }
ImVec2      getPos()      const { return pos; }
bool        isHovered()   const { return hovered; }
ImDrawList* getDrawList() const { return drawList; }

private:
    void content() override;
    void drawGizmo(Scene& scene, const SceneSelection& selection);
    int  raycast(Scene& scene, const glm::vec2& screenPos, float& dist, bool includeCameras = true);

    std::function<void(int)> onEntitySelection;

    ImVec2      size     = {0.0f, 0.0f};
    ImVec2      pos      = {0.0f, 0.0f};
    bool        hovered  = false;
    ImDrawList* drawList = nullptr;
};
