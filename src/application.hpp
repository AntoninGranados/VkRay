#pragma once

#include <functional>
#include <string>

#include "VkSmol/engine.hpp"
#include "VkSmol/frame_context.hpp"
#include "VkSmol/platform/platform.hpp"

#include "core/parameter_handler.hpp"

#include "core/core.hpp"
#include "offline/job_queue.hpp"

class Application {
public:
    explicit Application(Platform& platform);
    ~Application();

    void run();
    void runJobs(JobQueue& queue);

private:
    Platform&        platform;
    VkSmol           engine;
    ParameterHandler parameters;

    uint64_t lastSwapchainGeneration = 0;
    void initParameters();
    void initScene(const std::string& sceneFile = "assets/scenes/default.json");

    void renderFrame(std::function<void(FrameContext&)> onRender);
    void onFrameStart(float dt);
};
