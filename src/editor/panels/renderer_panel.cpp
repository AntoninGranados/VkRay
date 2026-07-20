#include "renderer_panel.hpp"

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/editor.hpp"
#include "editor/parameter_ui.hpp"
#include "editor/ui_constants.hpp"
#include "utils/log.hpp"

static void startRender(const char* paramPath, auto startFn) {
    auto& path = Core::getParameters().getPath(paramPath);
    if (path.empty()) {
        Log::error("Renderer", std::format("No output path set for '{}', configure it in the parameters panel.", paramPath));
        return;
    }
    Core::setOutputPath(path);
    Editor::getUi().clearEntitySelection();
    Editor::getUi().saveToggledState();
    Editor::getUi().setToggle(false);
    startFn();
}

void RendererPanel::content() {
    ui::setNextWindowFixed();
    ImGui::Begin(ICON_FA_CAMERA " Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_PLAY " Render", { -FLT_MIN, 0 }))
            startRender("renderer/output/output_image", Core::startRender);
        if (ImGui::Button(ICON_FA_FILM " Render Animation", { -FLT_MIN, 0 }))
            startRender("renderer/output/output_video", Core::startRenderAnim);
        if (ImGui::Button(ICON_FA_ROTATE " Reload Shaders", { -FLT_MIN, 0 }))
            Core::reloadShaders();

        ImGui::Separator();
        ParameterUI::drawGroup("renderer");
    }
    ImGui::End();
}
