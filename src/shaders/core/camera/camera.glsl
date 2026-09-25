#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "../inputs.glsl"
#include "../motion.glsl"
#include "../random/utils.glsl"
#include "aperture.glsl"
#include "../../generated/camera_lens_dispatch.glsl"

Ray getRay(vec2 ndc_pos, inout RngState rng) {
    CameraPose pose = sampleCameraPose(ubo.camera.motionOffset);

    if (ubo.camera.lensSlot >= 0) {
        LensSample lensSample = dispatchCameraLens(ubo.camera.lensSlot, ubo.camera.lensParamsBase, ndc_pos, pose, ubo.camera.U, ubo.camera.V, rng);
        return Ray(lensSample.origin, normalize(lensSample.direction));
    }

    if (ubo.camera.projection == projection_Orthographic)
        return cameraRay(ndc_pos, pose, ubo.camera.projection, ubo.camera.U, ubo.camera.V);

    vec3 U = pose.right * ubo.camera.U;
    vec3 V = pose.up * ubo.camera.V;
    vec3 focalPoint = pose.eye + (ndc_pos.x * U - ndc_pos.y * V + pose.dir) * ubo.camera.thinLens.focusDistance;

    vec3 offset = vec3(0.0);
    if (ubo.camera.thinLens.lensRadius > 0.0) {
        vec2 p = sampleLens(rng);
        offset = ubo.camera.thinLens.lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    vec3 origin = pose.eye + offset;
    return Ray(origin, normalize(focalPoint - origin));
}

#endif
