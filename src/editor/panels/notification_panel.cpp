#include "notification_panel.hpp"

#include "imgui/imgui.h"
#include "FontAwesome/IconsFontAwesome7.h"

#include "app/app_context.hpp"
#include "app/notification_handler.hpp"
#include "editor/ui_constants.hpp"

void NotificationPanel::draw(AppContext& ctx) {
    ImGui::SetNextWindowBgAlpha(ui::kWindowBgAlphaLight);
    ImGui::SetNextWindowPos({ 0, ImGui::GetMainViewport()->Size.y - 500 });
    ImGui::SetNextWindowSize({ 300, 500 });
    ImGui::Begin("Outputs",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus // | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs
    );
    {
        ImGui::BeginChild("MessagesRegion", { 0, -ImGui::GetFrameHeightWithSpacing() }, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        for (auto& n : ctx.notifications->getNotifications()) {
            std::string label = "";
            ImVec4 color;
            switch (n.type) {
                case NotificationType::Info:    label = "[INFO]";    color = { 0.55, 0.91, 0.99, 1.00 }; break;
                case NotificationType::Warning: label = "[WARNING]"; color = { 1.00, 0.72, 0.42, 1.00 }; break;
                case NotificationType::Error:   label = "[ERROR]";   color = { 1.00, 0.33, 0.33, 1.00 }; break;
                case NotificationType::Debug:   label = "[DEBUG]";   color = { 0.74, 0.58, 0.98, 1.00 }; break;
                case NotificationType::Command: label = ">";         color = { 0.00, 0.00, 0.00, 1.00 }; break;
                default: break;
            }

            std::string line = label + " " + n.content;

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopTextWrapPos();

            ImGui::GetWindowDrawList()->AddText(
                ImGui::GetItemRectMin(),
                ImGui::ColorConvertFloat4ToU32(color),
                label.c_str()
            );
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 5.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        static char buff[256];
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Input", buff, IM_ARRAYSIZE(buff), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            ctx.notifications->pushMessage(NotificationType::Command, buff);
            ctx.notifications->parseInput(buff);
            buff[0] = '\0';
        }
        ImGui::PopItemWidth();

    }
    ImGui::End();
}