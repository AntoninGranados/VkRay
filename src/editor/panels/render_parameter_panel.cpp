#include "render_parameter_panel.hpp"

#include "imgui/imgui.h"
#include "IconsFontAwesome7.h"
#include "editor/ui_constants.hpp"

#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"

void RenderParameterPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_GEAR " Render", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ctx.parameters->drawGroup("Pathtracer", *ctx.restartRender);
    }
    ImGui::End();
}
