#pragma once

#include <cstdint>
#include <optional>
#include <string>

class Scene;

namespace SceneSerializer {
    bool load(Scene& scene, const std::string& path, std::optional<uint32_t> seed = std::nullopt);
    bool save(Scene& scene, const std::string& path);
}
