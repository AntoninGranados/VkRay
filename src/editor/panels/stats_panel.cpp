#include "stats_panel.hpp"

#include <algorithm>
#include <array>

#include "imgui/imgui.h"

#include "core/core.hpp"
#include "editor/editor.hpp"

void StatsPanel::content() {
    static constexpr float kTargetMs  = 1000.0f / 60.0f;
    static constexpr int   kAvg       = 5;
    static constexpr float kCanvasW   = 300.0f;
    static constexpr float kCanvasH   = 90.0f;
    static constexpr float kPaddingTop = 6.0f;

    VkSmol& engine = Core::getEngine();
    CoreRenderer& core = Core::getCoreRenderer();
    EditorRenderer& editor = Editor::getEditorRenderer();

    const std::array<PassInfo, kNumPasses> passes = {{
        { "Pathtracing", IM_COL32(255,  70,  70, 255), core.getPathtracingTimestamp() },
        { "Compositing", IM_COL32( 70, 170, 255, 255), core.getCompositingTimestamp() },
        { "Display",     IM_COL32( 50, 220,  80, 255), editor.getDisplayTimestamp()   },
        { "Debug",       IM_COL32(255, 200,  40, 255), editor.getDebugTimestamp()     },
        { "UI",          IM_COL32(220,  80, 255, 255), editor.getUiTimestamp()        },
    }};

    FrameSample sample;
    const bool paused = core.isRenderFinished();
    sample.ms[0] = paused ? 0.0f : static_cast<float>(engine.getTimestampMs(passes[0].timestamp));
    for (int p = 1; p < kNumPasses; ++p)
        sample.ms[p] = static_cast<float>(engine.getTimestampMs(passes[p].timestamp));

    bool valid = paused || sample.ms[0] >= 0.0f;
    for (int p = 1; p < kNumPasses; ++p)
        if (sample.ms[p] < 0.0f) { valid = false; break; }

    if (valid) {
        for (float& v : sample.ms) v = std::max(v, 0.0f);
        history[historyHead] = sample;
        historyHead = (historyHead + 1) % kHistorySize;
        if (historyCount < kHistorySize) ++historyCount;
    }

    ImGui::SetNextWindowPos(Editor::getUi().getViewportPos(), ImGuiCond_Always);
    ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);
    ImGui::Text("%.1f fps (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("%u samples", core.getSampleCount());
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        showGraph = !showGraph;
    ImGui::End();

    if (!showGraph) return;

    float maxTotal = kTargetMs;
    for (int i = 0; i < historyCount; ++i) {
        int idx = (historyHead - historyCount + i + kHistorySize * 2) % kHistorySize;
        float total = 0.0f;
        for (float v : history[idx].ms) total += v;
        maxTotal = std::max(maxTotal, total);
    }

    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::Begin("GPU Timings", &showGraph, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(kCanvasW, kCanvasH));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(origin, ImVec2(origin.x + kCanvasW, origin.y + kCanvasH), IM_COL32(30, 30, 36, 255));

    const float barW = kCanvasW / kHistorySize;
    for (int i = 0; i < historyCount; ++i) {
        int idx = (historyHead - historyCount + i + kHistorySize * 2) % kHistorySize;
        const FrameSample& s = history[idx];

        float x0    = origin.x + i * barW;
        float x1    = x0 + barW;
        float yBase = origin.y + kCanvasH;

        for (int p = 0; p < kNumPasses; ++p) {
            float h = (s.ms[p] / maxTotal) * (kCanvasH - kPaddingTop);
            dl->AddRectFilled(ImVec2(x0, yBase - h), ImVec2(x1, yBase), passes[p].color);
            yBase -= h;
        }
    }

    float targetY = origin.y + kCanvasH - (kTargetMs / maxTotal) * (kCanvasH - kPaddingTop);
    dl->AddLine(ImVec2(origin.x, targetY), ImVec2(origin.x + kCanvasW, targetY), IM_COL32(255, 255, 255, 120));

    ImGui::Spacing();

    struct LabelEntry { int passIdx; float avgMs; };
    std::array<LabelEntry, kNumPasses> labels;
    for (int p = 0; p < kNumPasses; ++p) {
        float sum = 0.0f;
        int n = std::min(historyCount, kAvg);
        for (int j = 0; j < n; ++j) {
            int idx = (historyHead - 1 - j + kHistorySize * 2) % kHistorySize;
            sum += history[idx].ms[p];
        }
        labels[p] = { p, n > 0 ? sum / n : 0.0f };
    }
    std::sort(labels.begin(), labels.end(), [](const LabelEntry& a, const LabelEntry& b) {
        return a.avgMs > b.avgMs;
    });

    for (const auto& l : labels) {
        ImGui::ColorButton("##col", ImGui::ColorConvertU32ToFloat4(passes[l.passIdx].color),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(10, 10));
        ImGui::SameLine();
        ImGui::Text("%-14s %.3f ms", passes[l.passIdx].name, l.avgMs);
    }

    ImGui::End();
}
