#pragma once

#include "scene.hpp"
#include "../engine/engine.hpp"

void initEmpty(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initCornellBox(VkSmol &engine, Scene &scene, LightMode &lightMode);
void initRandomSpheres(VkSmol &engine, Scene &scene, LightMode &lightMode);
