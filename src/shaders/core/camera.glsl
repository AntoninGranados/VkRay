#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "inputs.glsl"
#include "random/utils.glsl"

vec2 sampleLens(inout RngState rng) {
    for (int i = 0; i < 16; i++) {
        vec2 p = vec2(rand(rng), rand(rng)) * 2.0 - 1.0;
        float mask = texture(lensSampler, p * 0.5 + 0.5).r;
        if (rand(rng) < mask) return p;
    }
    return vec2(0.0);
}

Ray getRay(Camera camera, vec2 ndc_pos, inout RngState rng) {
    vec3 camRight = normalize(camera.U);
    vec3 camUp = normalize(camera.V);

    vec3 focalPoint;
    if (ubo.camera.tiltShift.enabled != 0) {
        vec3 d = vec3(ndc_pos.x * length(camera.U), -ndc_pos.y * length(camera.V), 1.0);
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
        focalPoint = camera.eye
            + focalPoint_cam.x * camRight
            + focalPoint_cam.y * camUp
            + focalPoint_cam.z * camera.W;
    } else {
        focalPoint = camera.eye + (ndc_pos.x * camera.U - ndc_pos.y * camera.V + camera.W) * ubo.camera.thinLens.focusDistance;
    }

    vec3 offset = vec3(0.0);
    if (ubo.camera.thinLens.lensRadius > 0.0) {
        vec2 p = sampleLens(rng);
        offset = ubo.camera.thinLens.lensRadius * (camRight * p.x + camUp * p.y);
    }

    vec3 origin = camera.eye + offset;
    return Ray(origin, normalize(focalPoint - origin));
}

#endif
