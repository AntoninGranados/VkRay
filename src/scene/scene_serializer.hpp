#pragma once

#include <string>

class Scene;
enum LightMode : int;

class SceneSerializer {
public:
    static bool load(Scene& scene, LightMode& lightMode, const std::string& path);
    static bool save(Scene& scene, LightMode lightMode, const std::string& path);
};
