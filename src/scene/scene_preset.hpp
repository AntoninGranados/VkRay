#pragma once

#include "scene.hpp"
#include "../engine/engine.hpp"

enum class ScenePreset : int {
    Empty,
    Mesh,
    Sponza,
    CornellBox,
    RandomSpheres
};

void initEmpty(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initMesh(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initSponza(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initCornellBox(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initRandomSpheres(VkSmol &engine, Scene &scene, LightMode &lightMode);
