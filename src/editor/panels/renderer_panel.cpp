#include "renderer_panel.hpp"

#include <nfd.hpp>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/export_service.hpp"
#include "editor/editor.hpp"
#include "editor/parameter_ui.hpp"
#include "editor/ui_constants.hpp"

void RendererPanel::draw() {
    ui::setFixedDockClass();
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_CAMERA " Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_PLAY " Render", { -FLT_MIN, 0 }))
            if (promptImagePath()) {
                Editor::getUi().clearEntitySelection();
                Editor::getUi().saveToggledState();
                Editor::getUi().setToggle(false);
                Core::startRender();
            }
        if (ImGui::Button(ICON_FA_FILM " Render Animation", { -FLT_MIN, 0 }))
            if (promptVideoPath()) {
                Editor::getUi().clearEntitySelection();
                Editor::getUi().saveToggledState();
                Editor::getUi().setToggle(false);
                Core::startRenderAnim();
            }
        if (ImGui::Button(ICON_FA_ROTATE " Reload Shaders", { -FLT_MIN, 0 }))
            Core::reloadShaders();

        ImGui::Separator();
        ParameterUI::drawGroup(Core::getParameters(), "pathtracer");
    }
    ImGui::End();
}

bool RendererPanel::promptImagePath() {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdfilteritem_t filters[2] = {
        { "PNG Image",     "png" },
        { "OpenEXR Image", "exr" }
    };
    if (NFD::SaveDialog(outPath, filters, 2, kRenderOutputDir, "render") != NFD_OKAY)
        return false;
    std::filesystem::path p = outPath.get();
    if (!p.has_extension()) p.replace_extension(".png");
    Core::setOutputPath(std::move(p));
    return true;
}

bool RendererPanel::promptVideoPath() {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdfilteritem_t filters[1] = {
        { "MP4 Video",     "mp4" }
    };
    if (NFD::SaveDialog(outPath, filters, 1, kRenderOutputDir, "video") != NFD_OKAY)
        return false;
    std::filesystem::path p = outPath.get();
    if (!p.has_extension()) p.replace_extension(".mp4");
    Core::setOutputPath(std::move(p));
    return true;
}