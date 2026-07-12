#pragma once

#include <optional>
#include <string>

class Scene;
class VkSmol;
enum LightMode : int;

class SceneSerializer {
public:
    // TODO: remove support for in object material definition and allow for repeat/grid in the material definition
    static bool load(Scene& scene, VkSmol& engine, LightMode& lightMode, const std::string& path, std::optional<uint32_t> seed = std::nullopt);
    static bool save(Scene& scene, LightMode lightMode, const std::string& path);
};
