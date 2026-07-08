#pragma once

#include <functional>
#include <filesystem>

#include <glm/glm.hpp>

#include "structures.hpp"

class VkSmol;
class Scene;
class Camera;
class ParameterHandler;
class EditorUi;
class AnimationHandler;
class Platform;

// TODO: this GOD STRUCTURE should be removed ASAP
struct AppContext {
    VkSmol*           engine       = nullptr;
    Scene*            scene        = nullptr;
    Camera*           camera       = nullptr;
    ParameterHandler* parameters   = nullptr;
    EditorUi*         ui           = nullptr;
    AnimationHandler* animation    = nullptr;

    RenderState*    renderState    = nullptr;
    PathtracerUBO*  pathtracerUBO  = nullptr;
    CompositingUBO* compositingUBO = nullptr;
    DisplayUBO*     displayUBO     = nullptr;

    std::filesystem::path outputPath;
    bool*     restartRender = nullptr;
    Platform* platform      = nullptr;

    std::function<void()> reloadShaders;
    std::function<void()> startRender;
    std::function<void()> startRenderAnim;
};
