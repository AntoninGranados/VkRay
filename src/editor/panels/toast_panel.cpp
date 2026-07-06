#include "toast_panel.hpp"

#include <algorithm>
#include <cstdio>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "editor/ui_constants.hpp"

constexpr float kToastMaxMessageHeight = 56.f;
constexpr float kToastTTL     = 15.0f;
constexpr float kToastFadeOut = 0.2f;
constexpr float kToastWidth   = 400.0f;
constexpr float kToastPadding = 12.0f;

ToastPanel::ToastPanel() {
    Log::setConsumer([this](const LogEntry& e) { push(e); });
}

void ToastPanel::push(const LogEntry& entry) {
    toasts.push_back({ entry, kToastTTL });
}

void ToastPanel::draw() {
    auto  now = std::chrono::steady_clock::now();
    float dt  = std::chrono::duration<float>(now - lastTick).count();
    lastTick  = now;

    for (auto& t : toasts) t.timeLeft -= dt;
    std::erase_if(toasts, [](const Toast& t) { return t.timeLeft <= 0.0f; });
    if (toasts.empty()) return;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    float x = vp->Pos.x + kToastPadding;
    float y = vp->Pos.y + vp->Size.y - kToastPadding;

    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoDecoration          |
        ImGuiWindowFlags_NoNav                 |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoSavedSettings       |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing    |
        ImGuiWindowFlags_NoDocking;

    for (int i = (int)toasts.size() - 1; i >= 0; i--) {
        Toast& t = toasts[i];
        float alpha = std::min(1.0f, t.timeLeft / kToastFadeOut);

        const char* icon;
        const char* label;
        ImVec4 accent;
        switch (t.entry.level) {
            case LogLevel::Error:   icon = ICON_FA_CIRCLE_XMARK;       label = "Error";   accent = { 0.85f, 0.20f, 0.20f, alpha }; break;
            case LogLevel::Warn:    icon = ICON_FA_CIRCLE_EXCLAMATION; label = "Warning"; accent = { 0.85f, 0.65f, 0.10f, alpha }; break;
            case LogLevel::Success: icon = ICON_FA_CIRCLE_CHECK;       label = "Success"; accent = { 0.20f, 0.80f, 0.40f, alpha }; break;
            case LogLevel::Info:    icon = ICON_FA_CIRCLE_INFO;        label = "Info";    accent = { 0.40f, 0.60f, 0.90f, alpha }; break;
            default:                icon = ICON_FA_BUG;                label = "Debug";   accent = { 0.60f, 0.60f, 0.60f, alpha }; break;
        }

        char id[32];
        snprintf(id, sizeof(id), "##toast%d", i);

        float cachedH = 36.f;
        if (ImGuiWindow* win = ImGui::FindWindowByName(id)) cachedH = win->Size.y;
        y -= cachedH;

        ImGui::SetNextWindowPos({ x, y }, ImGuiCond_Always);
        ImGui::SetNextWindowSize({ kToastWidth, 0 }, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(alpha * ui::kWindowBgAlpha);
        ImGui::PushStyleColor(ImGuiCol_Border, accent);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.f, 7.f });

        ImGui::Begin(id, nullptr, kFlags);

        // Header row: icon · source · close button
        float btnW = ImGui::CalcTextSize(ICON_FA_XMARK).x + ImGui::GetStyle().FramePadding.x * 2.f;
        ImGui::TextColored(accent, "%s %s", icon, label);
        ImGui::SameLine();
        if (!t.entry.source.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 1.f, alpha * 0.55f });
            ImGui::TextUnformatted(t.entry.source.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - btnW);
        ImGui::PushStyleColor(ImGuiCol_Text,          { 1.f, 1.f, 1.f, alpha * 0.5f });
        ImGui::PushStyleColor(ImGuiCol_Button,        { 0.f, 0.f, 0.f, 0.f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 1.f, 1.f, 1.f, 0.15f });
        char closeId[48];
        snprintf(closeId, sizeof(closeId), ICON_FA_XMARK "##c%d", i);
        if (ImGui::SmallButton(closeId)) t.timeLeft = 0.f;
        ImGui::PopStyleColor(3);

        // Message below — scrollable, clipped to max height
        ImGui::PushStyleColor(ImGuiCol_ChildBg, { 0.f, 0.f, 0.f, 0.f });
        ImGui::BeginChild("##msg", { 0.f, kToastMaxMessageHeight });
        ImGui::PushTextWrapPos(0.f);
        ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 1.f, alpha });
        ImGui::TextUnformatted(t.entry.message.c_str());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        y -= kToastPadding;
    }
}
