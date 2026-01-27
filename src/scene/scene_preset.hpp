#pragma once

#include "scene.hpp"

enum class ScenePreset : int {
    Empty,
    Mesh,
    Sponza,
    CornellBox,
    RandomSpheres
};

void initEmpty(Scene &scene, LightMode &lightMode);
void initMesh(Scene &scene, LightMode &lightMode);
void initSponza(Scene &scene, LightMode &lightMode);
void initCornellBox(Scene &scene, LightMode &lightMode);
void initRandomSpheres(Scene &scene, LightMode &lightMode);
