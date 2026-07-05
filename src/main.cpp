#include <cstdlib>
#include <filesystem>
#include <string_view>

#include "VkSmol/platform/glfw_platform.hpp"
#include "VkSmol/platform/headless_platform.hpp"

#include "application.hpp"
#include "render/job_queue.hpp"

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string_view(argv[1]) == "--reference") {
        JobQueue queue = JobQueue::fromFile("res/jobs/reference.json");
        if (queue.isEmpty()) return EXIT_SUCCESS;
        HeadlessPlatform platform(queue.entries().front().width, queue.entries().front().height);
        Application app(platform);
        app.runJobs(queue);

    } else if (argc >= 3 && std::string_view(argv[1]) == "--job") {
        JobQueue queue = JobQueue::fromFile(argv[2]);
        if (queue.isEmpty()) return EXIT_SUCCESS;
        HeadlessPlatform platform(queue.entries().front().width, queue.entries().front().height);
        Application app(platform);
        app.runJobs(queue);
    
    } else {
        GLFWPlatform platform("VkRay", 1280, 720);
        Application app(platform);
        app.run();
    }
    return EXIT_SUCCESS;
}
