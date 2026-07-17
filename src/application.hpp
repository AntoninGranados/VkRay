#pragma once

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
    std::function<void()>     runFn;

    void initEditorMode();
    void initOfflineMode(const std::string& jobFile);

    void initParameters();
    void initScene(const std::string& sceneFile = "assets/scenes/default.json");
    void buildRenderGraph(bool withEditor);
};
