#include <filesystem>
#include <iostream>

#include "core/ecs/components.hpp"
#include "core/ecs/components/component_serializer.hpp"
#include "core/fields/parameter_serializer.hpp"
#include "editor/ecs/components/camera.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: VkRayDocs <output directory>" << std::endl;
        return 1;
    }

    const std::filesystem::path directory = argv[1];
    std::filesystem::create_directories(directory);

    const ParameterRegistry parameters = ParameterSerializer::load("builtin:/config/parameters.json");
    ParameterSerializer::saveDocumentation(parameters, directory / "parameters.md");
    ComponentSerializer::saveDocumentation(directory / "components.md");
    return 0;
}
