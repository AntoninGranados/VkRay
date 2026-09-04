#include "render_progress_panel.hpp"

#include <algorithm>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "core/animation/animation_clock.hpp"
#include "core/parameters/parameters.hpp"
#include "editor/ui_utils.hpp"

void RenderProgressPanel::draw() {
    ImGui::SetNextWindowPos({0, 0});
    ui::drawWindow(getTitle(),
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration,
        [this] {
            uint32_t sampleCount = Core::getCoreRenderer().getSampleCount();
            int renderSamplesPerPixel = Core::getParameters().get<int>("renderer/sampling/render_samples");

            if (sampleCount == 1) timer.start();

            if (renderSamplesPerPixel > 0) {
                uint32_t current = std::min(sampleCount, static_cast<uint32_t>(renderSamplesPerPixel));
                const ProgressStats stats = timer.stats(current, static_cast<uint32_t>(renderSamplesPerPixel));

                char overlay[64];
                snprintf(overlay, sizeof(overlay), "%u / %d", current, renderSamplesPerPixel);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.55f, 0.55f, 0.55f, 0.85f));
                ImGui::ProgressBar(stats.progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), "");
                ImGui::PopStyleColor();

                ImVec2 textSize = ImGui::CalcTextSize(overlay);
                ImVec2 barMin = ImGui::GetItemRectMin();
                ImVec2 barMax = ImGui::GetItemRectMax();
                ImVec2 textPos(
                    (barMin.x + barMax.x - textSize.x) * 0.5f,
                    (barMin.y + barMax.y - textSize.y) * 0.5f
                );
                ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), overlay);

                if (stats.progress > 0.0f) {
                    ImGui::Text("ETA: %s", ProgressTimer::formatTime(stats.eta).c_str());

                    if (Core::getRenderMode() == RenderMode::RenderAnimation) {
                        int totalFrames = std::max(1, Core::getAnimation().getEndFrame());
                        int currentFrameIdx = std::clamp(Core::getAnimation().getFrame(), 0, totalFrames - 1);
                        int remainingFrames = totalFrames - currentFrameIdx - 1;
                        double totalEta = stats.eta + remainingFrames * (stats.elapsed / stats.progress);
                        ImGui::Text("Total ETA: %s", ProgressTimer::formatTime(totalEta).c_str());
                    }
                } else {
                    ImGui::Text("ETA: --");
                    if (Core::getRenderMode() == RenderMode::RenderAnimation)
                        ImGui::Text("Total ETA: --");
                }
            }

            if (Core::getRenderMode() == RenderMode::RenderAnimation) {
                int totalFrames = std::max(1, Core::getAnimation().getEndFrame());
                int currentFrame = std::clamp(Core::getAnimation().getFrame(), 0, totalFrames - 1) + 1;
                ImGui::Text("Frame: %d / %d", currentFrame, totalFrames);
            }
        }
    );
}
