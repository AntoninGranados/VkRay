#include "app/notification_handler.hpp"

constexpr int MAX_NOTIFICATION_COUNT = 32;

void NotificationHandler::pushMessage(NotificationType type, std::string content) {
    notifications.push_back({
        .type = type,
        .content = content
    });
    if (notifications.size() > MAX_NOTIFICATION_COUNT)
        notifications.erase(notifications.begin());
}

void NotificationHandler::pushNotification(Notification notification) {
    notifications.push_back(notification);
    if (notifications.size() > MAX_NOTIFICATION_COUNT)
        notifications.erase(notifications.begin());
}

bool NotificationHandler::isCommandRequested(enum Command command) {
    if (requestedCommands[command]) {
        requestedCommands[command] = false;
        return true;
    }
    return false;
}

void NotificationHandler::parseInput(char* buff) {
    if (strcmp(buff, "clear") == 0) {
        requestedCommands[Command::Clear] = true;
        notifications.clear();
    } else if (strcmp(buff, "exit") == 0) {
        requestedCommands[Command::Exit] = true;
    } else if (strcmp(buff, "help") == 0) {
        requestedCommands[Command::Help] = true;
        pushHelp();
    } else if (strcmp(buff, "key") == 0) {
        requestedCommands[Command::Key] = true;
        pushKeymaps();
    } else if (strcmp(buff, "render") == 0) {
        requestedCommands[Command::Render] = true;
    } else if (strcmp(buff, "render-anim") == 0) {
        requestedCommands[Command::RenderAnim] = true;
    } else if (strcmp(buff, "reload") == 0) {
        requestedCommands[Command::Reload] = true;
    } else {
        notifications.push_back({ NotificationType::Error, "Unrecognised command" });
    }
}

void NotificationHandler::pushHelp() {
    notifications.push_back({ NotificationType::Info, "Available commands:" });
    char buff[128];
    for (auto& command : commands) {
        snprintf(buff, 128, "- %s: %s", command.first.c_str(), command.second.c_str());
        notifications.push_back({ NotificationType::Other, buff });
    }
}

void NotificationHandler::pushKeymaps() {
    notifications.push_back({ NotificationType::Info, "Keymaps:" });
    char buff[128];
    for (auto& keymap : keymaps) {
        snprintf(buff, 128, "- [%s]: %s", keymap.first.c_str(), keymap.second.c_str());
        notifications.push_back({ NotificationType::Other, buff });
    }
}
