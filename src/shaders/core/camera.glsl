#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "inputs.glsl"
#include "motion.glsl"
#include "random/utils.glsl"

vec2 sampleLens(inout RngState rng) {
    for (int i = 0; i < 16; i++) {
        vec2 p = vec2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float mask = texture(lensSampler, p * 0.5 + 0.5).r;
        if (rand(rng) < mask) return p;
    }
    return vec2(0.0);
}

Ray getRay(vec2 ndc_pos, inout RngState rng) {
    CameraPose pose = sampleCameraPose(ubo.camera.motionOffset);
    vec3 U = pose.right * ubo.camera.U;
    vec3 V = pose.up * ubo.camera.V;

    vec3 focalPoint;
    if (ubo.camera.tiltShift.enabled != 0) {
        vec3 d = vec3(ndc_pos.x * ubo.camera.U, -ndc_pos.y * ubo.camera.V, 1.0);
        vec3 ab = ubo.camera.tiltShift.focusB - ubo.camera.tiltShift.focusA;
        vec3 ac = ubo.camera.tiltShift.focusC - ubo.camera.tiltShift.focusA;
        vec3 planeNormal = normalize(cross(ab, ac));
        float dDotN = dot(d, planeNormal);
        vec3 focalPoint_cam;
        if (abs(dDotN) > 1e-6) {
            float t = dot(ubo.camera.tiltShift.focusA, planeNormal) / dDotN;
            focalPoint_cam = t * d;
        } else {
            focalPoint_cam = d * ubo.camera.thinLens.focusDistance;
        }
        focalPoint = pose.eye
            + focalPoint_cam.x * pose.right
            + focalPoint_cam.y * pose.up
            + focalPoint_cam.z * pose.dir;
    } else {
        focalPoint = pose.eye + (ndc_pos.x * U - ndc_pos.y * V + pose.dir) * ubo.camera.thinLens.focusDistance;
    }

    vec3 offset = vec3(0.0);
    if (ubo.camera.thinLens.lensRadius > 0.0) {
        vec2 p = sampleLens(rng);
        offset = ubo.camera.thinLens.lensRadius * (pose.right * p.x + pose.up * p.y);
    }

    vec3 origin = pose.eye + offset;
    return Ray(origin, normalize(focalPoint - origin));
}

#endif
