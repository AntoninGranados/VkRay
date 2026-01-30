#pragma once

namespace ecs {

struct CameraObject {
    float fov;
    float aperture;
    float focusDepth;
    bool isPreview;
    bool previewJustSet;
    bool updated;

    void setFov(float newFov) { fov = newFov; }
    void setAperture(float newAperture) { aperture = newAperture; }
    void setFocusDepth(float newFocusDepth) { focusDepth = newFocusDepth; }
    void setPreview(bool newIsPreview) { isPreview = newIsPreview; }
    void setPreviewJustSet(bool newPreviewJustSet) { previewJustSet = newPreviewJustSet; }
    void setUpdated(bool newUpdated) { updated = newUpdated; }
};

} // namespace ecs
