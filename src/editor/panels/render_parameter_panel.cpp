#include "render_parameter_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"

void RenderParameterPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_GEAR " Render", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ctx.parameters->drawGroup("pathtracer", *ctx.restartRender);
    }
    ImGui::End();
}
