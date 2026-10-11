import "../scene.slang";
import "../motion.slang";
import "../random/utils.slang";
import "aperture.slang";
import "../../generated/camera_lens_dispatch.slang";

Ray getRay(in CameraUBO camera, float2 ndc_pos, inout RngState rng) {
    CameraPose pose = sampleCameraPose(camera.motionOffset);

    if (camera.lensSlot >= 0) {
        LensSample lensSample = dispatchCameraLens(camera, ndc_pos, pose, rng);
        return Ray(lensSample.origin, normalize(lensSample.direction));
    }

    if (camera.projection == projection_Orthographic)
        return cameraRay(ndc_pos, pose, camera.projection, camera.U, camera.V);

    float3 U = pose.right * camera.U;
    float3 V = pose.up * camera.V;
    float3 focalPoint = pose.eye + (ndc_pos.x * U - ndc_pos.y * V + pose.dir) * camera.thinLens.focusDistance;

    float3 offset = float3(0.0);
    if (camera.thinLens.lensRadius > 0.0) {
        float2 p = sampleLens(rng);
        offset = camera.thinLens.lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    float3 origin = pose.eye + offset;
    return Ray(origin, normalize(focalPoint - origin));
}
