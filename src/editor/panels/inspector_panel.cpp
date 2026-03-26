#include "inspector_panel.hpp"

#include "imgui/imgui.h"
#include "editor/ui_constants.hpp"

#include "scene/scene.hpp"

void InspectorPanel::draw(AppContext& ctx) {
    Scene& scene = *ctx.scene;
    scene.drawSelectedEntityUI();
    scene.drawSelectedMaterialUI();
    scene.drawSelectedMeshAssetUI();
}
