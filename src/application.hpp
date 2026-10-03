#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include "VkSmol/platform/platform.hpp"

class Application {
public:
    Application(int argc, char* argv[]);
    ~Application();

    void run();

private:
    std::unique_ptr<Platform> platform;
    std::function<void()> runFn;

    void initEditorMode();
    void initOfflineMode(const std::string& jobFile);
    void generateDocumentation(const std::filesystem::path& directory);

    void initScene(const std::string& sceneFile = "builtin:/scenes/default.json");
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    void buildRenderGraph(bool offline);
};
