#pragma once

#include "imgui/imgui.h"

#include <vector>

enum NotificationType : int{
    Info,
    Warning,
    Error,
    Command,
    Debug,
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
    Key,
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
    void pushKeymaps();

    std::vector<Notification> notifications;
    bool requestedCommands[Command::Count] = {0};

    std::vector<std::pair<std::string, std::string> > commands = {
        { "clear", "clear the notifications" },
        { "exit", "exit the application" },
        { "help", "display this help message" },
        { "key", "display the keymaps" },
        { "render", "render mode (ESC to go to normal)" },
        { "reload", "reload the shaders" },
    };
    
    std::vector<std::pair<std::string, std::string> > keymaps = {
        { "RMB + W/A/S/D/SPACE/SHFT", "Fly mode (camera)" },
        { "MMB", "Orbit mode (camera)" },
        { "MMB + SHFT", "Pan mode (camera)" },
        { "MMB + CTRL", "Dolly mode (camera)" },
        { "SCROLL", "Change FOV" },
        { "R", "Reset render" },
        { "ESC", "Exit render mode / Unselect object" },
    };
};
