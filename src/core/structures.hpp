#pragma once

#include <glm/glm.hpp>

enum class RenderMode {
    Preview,
    RenderSingle,
    RenderAnimation
};

enum class DebugView : int {
    None = 0,
    PositionW,
    Position,
    NormalW,
    Normal,
    Albedo,
    Roughness,
    MatType,
    Bounces,
    HitChecks,
    Variance,
    SelectionMask,
    SkyMask,
};

