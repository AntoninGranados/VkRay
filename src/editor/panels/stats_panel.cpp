#include "stats_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/ui_constants.hpp"

void StatsPanel::draw() {
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );

    ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("%u samples", Core::getCoreRenderer().getSampleCount());

    ImGui::End();
}
