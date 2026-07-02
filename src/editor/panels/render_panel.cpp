#include "render_panel.hpp"

#include <algorithm>

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "app/animation_handler.hpp"
#include "app/parameter_handler.hpp"
#include "editor/ui_constants.hpp"

void RenderPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlpha);
    ImGui::Begin(ICON_FA_STOPWATCH " Loading",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration
    );
    int renderSamplesPerPixel = ctx.parameters->getInt("pathtracer/sampling/render_samples");
    if (renderSamplesPerPixel > 0) {
        float progress = static_cast<float>(std::min<uint64_t>(ctx.renderState->sampleCount, renderSamplesPerPixel))
            / static_cast<float>(renderSamplesPerPixel);
        char overlay[64];
        snprintf(
            overlay,
            sizeof(overlay),
            "%llu / %d",
            static_cast<unsigned long long>(std::min<uint64_t>(ctx.renderState->sampleCount, renderSamplesPerPixel)),
            renderSamplesPerPixel
        );
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
    }
    float samplesPerSec = static_cast<float>(ctx.renderState->samplesPerSecEMA);
    ImGui::Text("%.1f samples/sec", samplesPerSec);
    if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
        int totalFrames = std::max(1, ctx.animation->getEndFrame());
        int currentFrame = std::clamp(ctx.animation->getFrame(), 0, totalFrames - 1) + 1;
        ImGui::Text("Frame: %d / %d", currentFrame, totalFrames);
    }

    if (renderSamplesPerPixel > 0 && samplesPerSec > 0.0f) {
        uint64_t remaining = 0;
        if (ctx.renderState->sampleCount < static_cast<uint64_t>(renderSamplesPerPixel)) {
            remaining = static_cast<uint64_t>(renderSamplesPerPixel) - ctx.renderState->sampleCount;
        }
        float etaSec = static_cast<float>(remaining) / samplesPerSec;
        int etaMin = static_cast<int>(etaSec / 60.0f);
        int etaRemSec = static_cast<int>(etaSec) % 60;
        ImGui::Text("ETA: %dm %02ds", etaMin, etaRemSec);

        if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
            const int totalFrames = std::max(1, ctx.animation->getEndFrame());
            const int currentFrameIdx = std::clamp(ctx.animation->getFrame(), 0, totalFrames - 1);
            const uint64_t remainingFramesAfterCurrent = static_cast<uint64_t>(std::max(0, totalFrames - currentFrameIdx - 1));
            const uint64_t remainingTotalSamples = remaining + remainingFramesAfterCurrent * static_cast<uint64_t>(renderSamplesPerPixel);

            const float totalEtaSec = static_cast<float>(remainingTotalSamples) / samplesPerSec;
            const int totalEtaHour = static_cast<int>(totalEtaSec / 3600.0f);
            const int totalEtaMin = (static_cast<int>(totalEtaSec) % 3600) / 60;
            const int totalEtaRemSec = static_cast<int>(totalEtaSec) % 60;
            ImGui::Text("Total ETA: %dh %02dm %02ds", totalEtaHour, totalEtaMin, totalEtaRemSec);
        }
    } else {
        ImGui::Text("ETA: --");
        if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
            ImGui::Text("Total ETA: --");
        }
    }
    ImGui::End();
}