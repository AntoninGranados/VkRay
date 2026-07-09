#include "render_progress_panel.hpp"

#include <algorithm>

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "app/animation_handler.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"

void RenderProgressPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_STOPWATCH " Loading",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );

    uint32_t sampleCount = ctx.getSampleCount();
    int renderSamplesPerPixel = ctx.parameters->getInt("pathtracer/sampling/render_samples");

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

            if (ctx.renderMode == RenderMode::RenderAnimation) {
                int totalFrames = std::max(1, ctx.animation->getEndFrame());
                int currentFrameIdx = std::clamp(ctx.animation->getFrame(), 0, totalFrames - 1);
                int remainingFrames = totalFrames - currentFrameIdx - 1;
                double totalEta = timer.eta(progress) + remainingFrames * (timer.elapsed() / progress);
                ImGui::Text("Total ETA: %s", ProgressTimer::formatTime(totalEta).c_str());
            }
        } else {
            ImGui::Text("ETA: --");
            if (ctx.renderMode == RenderMode::RenderAnimation)
                ImGui::Text("Total ETA: --");
        }
    }

    if (ctx.renderMode == RenderMode::RenderAnimation) {
        int totalFrames = std::max(1, ctx.animation->getEndFrame());
        int currentFrame = std::clamp(ctx.animation->getFrame(), 0, totalFrames - 1) + 1;
        ImGui::Text("Frame: %d / %d", currentFrame, totalFrames);
    }

    ImGui::End();
}
