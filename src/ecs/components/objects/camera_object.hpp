#pragma once

namespace ecs {

struct CameraObject {
    float fov = 60.0f;
    float aperture = 0.0f;
    float focusDepth = 1.0f;
    bool isPreview = false;
    bool previewJustSet = false;
    bool updated = false;

    void setFov(float newFov) { fov = newFov; }
    void setAperture(float newAperture) { aperture = newAperture; }
    void setFocusDepth(float newFocusDepth) { focusDepth = newFocusDepth; }
    void setPreview(bool newIsPreview) { isPreview = newIsPreview; }
    void setPreviewJustSet(bool newPreviewJustSet) { previewJustSet = newPreviewJustSet; }
    void setUpdated(bool newUpdated) { updated = newUpdated; }
};

} // namespace ecs
