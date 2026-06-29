#include <cstdlib>
#include <string_view>

#include "VkSmol/platform/glfw_platform.hpp"
#include "VkSmol/platform/headless_platform.hpp"
#include "application.hpp"

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string_view(argv[1]) == "--reference") {
        const std::string output = argc >= 3 ? argv[2] : "reference.png";
        HeadlessPlatform platform(1080, 1080);
        Application app(platform);
        app.runHeadless("scenes/reference_scene.json", 8192, output);
    } else {
        GLFWPlatform platform("VkRay", 1280, 720);
        Application app(platform);
        app.run();
    }
    return EXIT_SUCCESS;
}
