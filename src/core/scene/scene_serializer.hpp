#pragma once

#include <cstdint>
#include <optional>
#include <string>

class Scene;
enum LightMode : int;

namespace SceneSerializer {
    bool load(Scene& scene, LightMode& lightMode, const std::string& path, std::optional<uint32_t> seed = std::nullopt);
    bool save(Scene& scene, LightMode lightMode, const std::string& path);
}
