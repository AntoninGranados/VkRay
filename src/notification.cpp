#include "notification.hpp"

void NotificationManager::drawNotifications() {
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::SetNextWindowPos({ 0, ImGui::GetMainViewport()->Size.y - 500 });
    ImGui::SetNextWindowSize({ 300, 500 });
    ImGui::Begin("Outputs",
        nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus // | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs
    );
    {
        ImGui::BeginChild("MessagesRegion", { 0, -ImGui::GetFrameHeightWithSpacing() }, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        for (auto& n : notifications) {
            const char* label = "";
            ImVec4 color;
            switch (n.type) {
                case NotificationType::Info:    label = "[INFO]";    color = { 0.55, 0.91, 0.99, 1.00 }; break;
                case NotificationType::Warning: label = "[WARNING]"; color = { 1.00, 0.72, 0.42, 1.00 }; break;
                case NotificationType::Error:   label = "[ERROR]";   color = { 1.00, 0.33, 0.33, 1.00 }; break;
                case NotificationType::Command: label = ">";         color = { 0.00, 0.00, 0.00, 1.00 }; break;
                default: break;
            }

            /*
            const float wrapPos = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
            ImGui::PushTextWrapPos(wrapPos);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            ImGui::TextUnformatted(n.content.c_str());
            ImGui::PopTextWrapPos();
            */

            std::string line = std::string(label) + " " + n.content;

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopTextWrapPos();

            ImGui::GetWindowDrawList()->AddText(
                ImGui::GetItemRectMin(),
                ImGui::ColorConvertFloat4ToU32(color),
                label
            );
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 5.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();

        static char buff[256];
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Input", buff, IM_ARRAYSIZE(buff), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            notifications.push_back({ NotificationType::Command, buff });
            parseInput(buff);
            buff[0] = '\0';
        }
        ImGui::PopItemWidth();

    }
    ImGui::End();
}

void NotificationManager::pushMessage(NotificationType type, std::string content) {
    notifications.push_back({
        .type = type,
        .content = content
    });
}

void NotificationManager::pushNotification(Notification notification) {
    notifications.push_back(notification);
}

bool NotificationManager::isCommandRequested(enum Command command) {
    if (requestedCommands[command]) {
        requestedCommands[command] = false;
        return true;
    }
    return false;
}

void NotificationManager::parseInput(char *buff) {
    if (strcmp(buff, "clear") == 0) {
        requestedCommands[Command::Clear] = true;
        notifications.clear();
    } else if (strcmp(buff, "exit") == 0) {
        requestedCommands[Command::Exit] = true;
    } else if (strcmp(buff, "help") == 0) {
        requestedCommands[Command::Help] = true;
        pushHelp();
    } else if (strcmp(buff, "render") == 0) {
        requestedCommands[Command::Render] = true;
    } else if (strcmp(buff, "reload") == 0) {
        requestedCommands[Command::Reload] = true;
    } else {
        notifications.push_back({ NotificationType::Error, "Unrecognised command" });
    }
}

void NotificationManager::pushHelp() {
    notifications.push_back({ NotificationType::Info, "Available commands:" });
    char buff[128];
    for (auto &command : commands) {
        snprintf(buff, 128, "- %s: %s", command.first.c_str(), command.second.c_str());
        notifications.push_back({ NotificationType::Other, buff });
    }
}
