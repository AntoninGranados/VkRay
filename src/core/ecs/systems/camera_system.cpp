#include "camera_system.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/animation/animation_clock.hpp"
#include "core/camera/camera.hpp"
#include "core/core.hpp"

namespace ecs {

void cameraPreUpdateSystem(Registry& registry) {
    ::Camera& sceneCamera = *registry.ctx().get<::Camera*>();
    auto& cameras = registry.storage(Camera);
    auto& transforms = registry.storage(Transform);
    auto& thinLensCameras = registry.storage(ThinLens);
    auto& tiltShiftCameras = registry.storage(TiltShiftLens);

    if (sceneCamera.hasPreviewCamera()) {
        const ecs::Entity previewEnt = sceneCamera.getPreviewCamera();
        if (!cameras.has(previewEnt) || !transforms.has(previewEnt)) return;

        const Component& t = transforms.get(previewEnt);
        const Component& c = cameras.get(previewEnt);
        const glm::quat q = glm::quat(glm::radians(t.get<glm::vec3>("rotation")));
        const glm::vec3 pos = t.get<glm::vec3>("position");
        const float dist = glm::max(0.1f, glm::length(sceneCamera.getTarget() - sceneCamera.getPosition()));
        sceneCamera.setPosition(pos);
        sceneCamera.setTarget(pos + glm::normalize(q * glm::vec3(0.0f, 0.0f, -1.0f)) * dist);
        const float fps = static_cast<float>(Core::getAnimation().getFps());
        sceneCamera.setShutterSpeed(::Camera::blurFractionFromShutter(c.get<float>("shutter_speed"), fps));

        if (thinLensCameras.has(previewEnt)) {
            const Component& tl = thinLensCameras.get(previewEnt);
            const float focalLength = tl.get<float>("focal_length");
            const float fStop = tl.get<float>("f_stop");
            sceneCamera.setFov(::Camera::fovFromFocalLength(focalLength));
            sceneCamera.setDrawFocusPlane(tl.get<bool>("show_focus_plane"));

            sceneCamera.setThinLens({ focalLength, fStop, tl.get<float>("focal_distance") });
            if (tiltShiftCameras.has(previewEnt)) {
                const Component& ts = tiltShiftCameras.get(previewEnt);
                sceneCamera.setTiltShift(ts.get<glm::vec3>("plane_position"), ts.get<glm::vec3>("plane_rotation"));
            } else {
                sceneCamera.clearTiltShift();
            }
        } else {
            sceneCamera.setFov(c.get<float>("fov"));
            sceneCamera.clearLens();
        }
        return;
    }

    if (Core::getRenderMode() == RenderMode::Preview) {
        sceneCamera.clearLens();
        return;
    }

    const float fps = static_cast<float>(Core::getAnimation().getFps());

    for (const auto& e : cameras.entities()) {
        if (!transforms.has(e)) continue;
        const Component& c = cameras.get(e);

        const Component& t = transforms.get(e);
        const glm::vec3 pos = t.get<glm::vec3>("position");
        const glm::vec3 dir = glm::normalize(
            glm::quat(glm::radians(t.get<glm::vec3>("rotation"))) * glm::vec3(0.0f, 0.0f, -1.0f));
        sceneCamera.setPosition(pos);
        sceneCamera.setShutterSpeed(::Camera::blurFractionFromShutter(c.get<float>("shutter_speed"), fps));

        if (thinLensCameras.has(e)) {
            const Component& tl = thinLensCameras.get(e);
            const float focalLength = tl.get<float>("focal_length");
            const float fStop = tl.get<float>("f_stop");
            sceneCamera.setFov(::Camera::fovFromFocalLength(focalLength));
            sceneCamera.setDrawFocusPlane(tl.get<bool>("show_focus_plane"));

            const float focalDistance = glm::max(0.1f, tl.get<float>("focal_distance"));
            sceneCamera.setTarget(pos + dir * focalDistance);
            sceneCamera.setThinLens({ focalLength, fStop, focalDistance });
            if (tiltShiftCameras.has(e)) {
                const Component& ts = tiltShiftCameras.get(e);
                sceneCamera.setTiltShift(ts.get<glm::vec3>("plane_position"), ts.get<glm::vec3>("plane_rotation"));
            } else {
                sceneCamera.clearTiltShift();
            }
        } else {
            sceneCamera.setFov(c.get<float>("fov"));
            sceneCamera.setTarget(pos + dir * 10.0f);
            sceneCamera.clearLens();
        }
        break;
    }
}

void syncPreviewCameraToEntity(const ::Camera& cam, Registry& registry) {
    if (!cam.hasPreviewCamera()) return;
    const ecs::Entity e = cam.getPreviewCamera();
    auto& transforms = registry.storage(Transform);
    if (!transforms.has(e)) return;
    Component& t = transforms.get(e);
    const glm::quat q = glm::quatLookAt(glm::normalize(cam.getDirection()), cam.getUp());
    t.set<glm::vec3>("position", cam.getPosition());
    t.set<glm::vec3>("rotation", glm::degrees(glm::eulerAngles(q)));
}

} // namespace ecs
