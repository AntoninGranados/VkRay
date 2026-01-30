#pragma once

namespace ecs {

struct CameraObject {
    float fov;
    float aperture;
    float focusDepth;

    void setFov(float newFov) { fov = newFov; }
    void setAperture(float newAperture) { aperture = newAperture; }
    void setFocusDepth(float newFocusDepth) { focusDepth = newFocusDepth; }
};

} // namespace ecs
