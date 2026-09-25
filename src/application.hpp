#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "VkSmol/platform/platform.hpp"

class Application {
public:
    Application(int argc, char* argv[]);
    ~Application();

    void run();

private:
    std::unique_ptr<Platform> platform;
    std::function<void()>     runFn;
    std::vector<size_t>       shaderWatchIds;

    void initEditorMode();
    void initOfflineMode(const std::string& jobFile);

    void initScene(const std::string& sceneFile = "assets/scenes/default.json");
    void buildRenderGraph(bool offline);
};
