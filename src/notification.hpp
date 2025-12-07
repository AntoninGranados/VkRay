#pragma once

#include "imgui/imgui.h"

#include <vector>

enum NotificationType : int{
    Info,
    Warning,
    Error,
    Command,
    Other,
};

struct Notification {
    NotificationType type;
    std::string content;
};

enum Command : int {
    Clear,
    Exit,
    Help,
    Render,
    Reload,

    Count,
};

class NotificationManager {
public:
    void drawNotifications();
    
    void pushMessage(NotificationType type, std::string content);
    void pushNotification(Notification notification);

    bool isCommandRequested(enum Command command);

private:
    void parseInput(char *buff);
    void pushHelp();

    std::vector<Notification> notifications;
    bool requestedCommands[Command::Count] = {0};

    std::vector<std::pair<std::string, std::string> > commands = {
        { "clear", "clear the notifications" },
        { "exit", "exit the application" },
        { "help", "display this help message" },
        { "render", "render mode (ESC to go to normal)" },
        { "reload", "reload the shaders" },
    };
};
