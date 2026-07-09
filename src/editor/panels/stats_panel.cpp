#include "stats_panel.hpp"

#include "imgui/imgui.h"
#include "editor/ui_constants.hpp"

void StatsPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );

    ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("%u samples", ctx.getSampleCount());

    ImGui::End();
}
