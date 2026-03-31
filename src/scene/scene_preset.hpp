#pragma once

#include <map>
#include <string>
#include <functional>

#include "scene.hpp"

enum class ScenePreset : int {
    Empty,
    MaterialZoo,
    Mesh,
    Sponza,
    CornellBox,
    RandomSpheres
};

using PresetInit = std::function<void(Scene&, LightMode&)>;

void initEmpty(Scene& scene, LightMode& lightMode);
void initPyramid(Scene& scene, LightMode& lightMode);
void initMaterialZoo(Scene& scene, LightMode& lightMode);
void initMesh(Scene& scene, LightMode& lightMode);
void initSponza(Scene& scene, LightMode& lightMode);
void initCornellBox(Scene& scene, LightMode& lightMode);
void initRandomSpheres(Scene& scene, LightMode& lightMode);

static std::map<ScenePreset, std::string> scenePresetName = {
    { ScenePreset::Empty,         "Empty" },
    { ScenePreset::MaterialZoo,   "Material Zoo" },
    { ScenePreset::Mesh,          "Mesh" },
    { ScenePreset::Sponza,        "Sponza" },
    { ScenePreset::CornellBox,    "Cornell Box" },
    { ScenePreset::RandomSpheres, "Random Spheres" },
};

static std::map<ScenePreset,PresetInit> scenePresetInitMethod = {
    { ScenePreset::Empty,         initEmpty },
    { ScenePreset::MaterialZoo,   initMaterialZoo },
    { ScenePreset::Mesh,          initMesh },
    { ScenePreset::Sponza,        initSponza },
    { ScenePreset::CornellBox,    initCornellBox },
    { ScenePreset::RandomSpheres, initRandomSpheres },
};
