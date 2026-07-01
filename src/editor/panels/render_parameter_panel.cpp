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
        ctx.parameters->drawGroup("Pathtracer", *ctx.restartRender);

        if (ImGui::CollapsingHeader("AOVs")) {
            AovConfig& aov = *ctx.aovConfig;
            ImGui::Checkbox("Normal",           &aov.normal);
            ImGui::Checkbox("Normal (opaque)",  &aov.normalOpaque);
            ImGui::Checkbox("Albedo",           &aov.albedo);
            ImGui::Checkbox("Albedo (opaque)",  &aov.albedoOpaque);
            ImGui::Checkbox("Depth",            &aov.depth);
            ImGui::Checkbox("Depth (opaque)",   &aov.depthOpaque);
            ImGui::Checkbox("Sky mask",         &aov.skyMask);
            ImGui::Checkbox("Sky mask (opaque)",&aov.skyMaskOpaque);
        }
    }
    ImGui::End();
}
