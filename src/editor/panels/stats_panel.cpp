#include "stats_panel.hpp"

#include "imgui/imgui.h"

#include "core/core.hpp"

void StatsPanel::content() {
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::Begin("FPS",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );

    ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("%u samples", Core::getCoreRenderer().getSampleCount());

    ImGui::End();
}
