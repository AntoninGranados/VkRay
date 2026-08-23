#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/ecs/entity.hpp"

struct ThinLensState {
    float normalizedFocalLength = 0.0f;
    float fStop                 = 0.0f;
    float focusDistance         = 10.0f;
};

struct TiltShiftState {
    bool      enabled = false;
    glm::vec3 focusA  = { -1.0f, 0.0f, 5.0f };
    glm::vec3 focusB  = {  1.0f, 0.0f, 5.0f };
    glm::vec3 focusC  = {  0.0f, 1.0f, 5.0f };
};

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f));

    bool cursorPosCallback(double x, double y);
    bool scrollCallback(double xoffset, double yoffset);
    bool processInput(float deltaTime);

    float getTanHFov() const { return glm::tan(glm::radians(getFov()) * 0.5f); }

    static float fovFromFocalLength(float normalizedFocalLength) {
        return 2.0f * glm::degrees(glm::atan(0.5f / normalizedFocalLength));
    }
    static float focalLengthFromFov(float fovDegrees) {
        return 0.5f / glm::tan(glm::radians(fovDegrees) * 0.5f);
    }
    static float lensRadiusFromFStop(float normalizedFocalLength, float fStop) {
        return fStop > 0.0f ? normalizedFocalLength / (2.0f * fStop) : 0.0f;
    }
    static float blurFractionFromShutter(float seconds, float fps) {
        return seconds * fps;
    }
    glm::vec3 getDirection() const { return glm::normalize(target - position); };
    glm::vec3 getUp() const { return up; }
    glm::mat4 getView() const { return glm::lookAt(position, target, getUp()); };
    glm::mat4 getProjection(float aspect) const;

    glm::vec3 getTarget() const { return target; }
    void setTarget(glm::vec3 newTarget) { target = newTarget; }

    glm::vec3 getPosition() const { return position; }
    void setPosition(glm::vec3 newPosition) { position = newPosition; }

    float getFov() const { return fov; }
    void setFov(const float newFov) { fov = glm::clamp(newFov, 1.0f, 160.0f); }

    float getLensRadius() const { return lensRadiusFromFStop(thinLens.normalizedFocalLength, thinLens.fStop); }
    float getFocusDistance() const { return thinLens.focusDistance; }

    void setDrawFocusPlane(bool v) { drawFocusPlane = v; }

    void setThinLens(ThinLensState state) { thinLens = state; }
    void setFocusDistance(float d) { thinLens.focusDistance = d; }

    TiltShiftState getTiltShift() const { return tiltShift; }
    void setTiltShift(glm::vec3 planePosition, glm::vec3 planeRotationEuler);
    void clearTiltShift() { tiltShift = {}; }

    void clearLens() { drawFocusPlane = false; thinLens = {}; tiltShift = {}; }

    float getShutterSpeed() const { return shutterSpeed; }
    void setShutterSpeed(float newShutterSpeed) { shutterSpeed = newShutterSpeed; }

    bool isLocked() { return locked; }

    void resetMouse() { firstMouse = true; }

    void setPreviewCamera(ecs::Entity e) {
        if (!previewEntity.has_value())
            savedState = SavedState{ position, target, fov };
        previewEntity = e;
    }
    bool clearPreviewCamera() {
        if (!previewEntity.has_value()) return false;
        previewEntity.reset();
        if (savedState.has_value()) {
            position = savedState->position;
            target = savedState->target;
            fov = savedState->fov;
            savedState.reset();
        }
        return true;
    }
    bool hasPreviewCamera() const { return previewEntity.has_value(); }
    ecs::Entity getPreviewCamera() const { return *previewEntity; }
    bool isPreviewEntity(ecs::Entity e) const { return previewEntity.has_value() && *previewEntity == e; }

private:
    enum class DragMode {
        None,
        Look,
        Orbit,
        Pan,
        Dolly
    };

    struct SavedState {
        glm::vec3 position;
        glm::vec3 target;
        float fov;
    };

    float orbitDistance = 10.0f;
    bool drawFocusPlane = false;

    glm::vec3 position;
    glm::vec3 target;
    float fov = 80.0f;
    float shutterSpeed = 0.0f;
    ThinLensState thinLens;
    TiltShiftState tiltShift;

    float yaw   = 90.0f;
    float pitch = 0.0f;

    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;

    float speed = 20.0f;
    float sensitivity = 0.2f;
    float panSensitivity = 0.003f;
    float dollySensitivity = 0.01f;

    DragMode dragMode = DragMode::None;
    bool locked = true;
    std::optional<ecs::Entity> previewEntity;
    std::optional<SavedState> savedState;

    static constexpr glm::vec3 up = { 0.0f, 1.0f, 0.0f };

    static void updateYawPitchFromDirection(const glm::vec3& dir, float& yaw, float& pitch);
};
