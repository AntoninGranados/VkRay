#include "renderer_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"

void RendererPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_CAMERA " Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        if (ImGui::Button(ICON_FA_PLAY " Render", { -FLT_MIN, 0 }))
            if (ctx.startRender) ctx.startRender();
        if (ImGui::Button(ICON_FA_FILM " Render Animation", { -FLT_MIN, 0 }))
            if (ctx.startRenderAnim) ctx.startRenderAnim();
        if (ImGui::Button(ICON_FA_ROTATE " Reload Shaders", { -FLT_MIN, 0 }))
            if (ctx.reloadShaders) ctx.reloadShaders();

        ImGui::Separator();
        ctx.parameters->drawGroup("pathtracer", *ctx.restartRender);
    }
    ImGui::End();
}
