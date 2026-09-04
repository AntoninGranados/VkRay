#include "renderer_panel.hpp"

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/editor.hpp"
#include "editor/parameter_ui.hpp"
#include "editor/ui_utils.hpp"
#include "utils/log.hpp"

static void startRender(const char* paramPath, auto startFn) {
    auto path = Core::getParameters().get<std::filesystem::path>(paramPath);
    if (path.empty()) {
        Log::error("Renderer", std::format("No output path set for '{}', configure it in the parameters panel.", paramPath));
        return;
    }
    Core::setOutputPath(path);
    Editor::selectEntity(std::nullopt);
    Core::getScene().activateSceneCamera();
    startFn();
}

void RendererPanel::draw() {
    ui::setNextWindowFixed();
    ui::drawWindow(getTitle(), ImGuiWindowFlags_AlwaysAutoResize, [] {
        if (ImGui::Button(ICON_FA_PLAY " Render", { -FLT_MIN, 0 }))
            startRender("renderer/output/output_image", Core::startRender);
        if (ImGui::Button(ICON_FA_FILM " Render Animation", { -FLT_MIN, 0 }))
            startRender("renderer/output/output_video", Core::startRenderAnim);

        ImGui::Separator();
        ParameterUI::drawGroup("renderer");
    });
}
