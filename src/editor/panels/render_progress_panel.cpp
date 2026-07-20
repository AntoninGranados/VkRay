#include "render_progress_panel.hpp"

#include <algorithm>

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "core/core.hpp"
#include "core/animation_handler.hpp"
#include "core/parameters/parameters.hpp"

void RenderProgressPanel::content() {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::Begin(ICON_FA_STOPWATCH " Loading",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );

    uint32_t sampleCount = Core::getCoreRenderer().getSampleCount();
    int renderSamplesPerPixel = Core::getParameters().get<int>("renderer/sampling/render_samples");

    if (sampleCount == 1) timer.start();

    if (renderSamplesPerPixel > 0) {
        uint32_t current = std::min(sampleCount, static_cast<uint32_t>(renderSamplesPerPixel));
        float progress = static_cast<float>(current) / static_cast<float>(renderSamplesPerPixel);

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "%u / %d", current, renderSamplesPerPixel);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.55f, 0.55f, 0.55f, 0.85f));
        ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f), "");
        ImGui::PopStyleColor();

        ImVec2 textSize = ImGui::CalcTextSize(overlay);
        ImVec2 barMin = ImGui::GetItemRectMin();
        ImVec2 barMax = ImGui::GetItemRectMax();
        ImVec2 textPos(
            (barMin.x + barMax.x - textSize.x) * 0.5f,
            (barMin.y + barMax.y - textSize.y) * 0.5f
        );
        ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), overlay);

        if (progress > 0.0f) {
            ImGui::Text("ETA: %s", ProgressTimer::formatTime(timer.eta(progress)).c_str());

            if (Core::getRenderMode() == RenderMode::RenderAnimation) {
                int totalFrames = std::max(1, Core::getAnimation().getEndFrame());
                int currentFrameIdx = std::clamp(Core::getAnimation().getFrame(), 0, totalFrames - 1);
                int remainingFrames = totalFrames - currentFrameIdx - 1;
                double totalEta = timer.eta(progress) + remainingFrames * (timer.elapsed() / progress);
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

    ImGui::End();
}
