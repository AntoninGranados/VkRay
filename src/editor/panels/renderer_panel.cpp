#include "renderer_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include <nfd.hpp>

#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"
#include "core/export_service.hpp"

void RendererPanel::draw(AppContext& ctx) {
    ui::setFixedDockClass();
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_CAMERA " Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_PLAY " Render", { -FLT_MIN, 0 }))
            if (ctx.startRender && promptImagePath(ctx)) ctx.startRender();
        if (ImGui::Button(ICON_FA_FILM " Render Animation", { -FLT_MIN, 0 }))
            if (ctx.startRenderAnim && promptVideoPath(ctx)) ctx.startRenderAnim();
        if (ImGui::Button(ICON_FA_ROTATE " Reload Shaders", { -FLT_MIN, 0 }))
            if (ctx.reloadShaders) ctx.reloadShaders();

        ImGui::Separator();
        ctx.parameters->drawGroup("pathtracer", *ctx.restartRender);
    }
    ImGui::End();
}

bool RendererPanel::promptImagePath(AppContext& ctx) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdfilteritem_t filters[2] = {
        { "PNG Image",     "png" },
        { "OpenEXR Image", "exr" }
    };
    if (NFD::SaveDialog(outPath, filters, 2, kRenderOutputDir, "render") != NFD_OKAY)
        return false;
    ctx.outputPath = outPath.get();
    if (!ctx.outputPath.has_extension()) ctx.outputPath.replace_extension(".png");
    return true;
}

bool RendererPanel::promptVideoPath(AppContext& ctx) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdfilteritem_t filters[1] = {
        { "MP4 Video",     "mp4" }
    };
    if (NFD::SaveDialog(outPath, filters, 1, kRenderOutputDir, "video") != NFD_OKAY)
        return false;
    ctx.outputPath = outPath.get();
    if (!ctx.outputPath.has_extension()) ctx.outputPath.replace_extension(".mp4");
    return true;
}