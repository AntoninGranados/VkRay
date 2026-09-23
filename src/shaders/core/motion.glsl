#ifndef MOTION_GLSL
#define MOTION_GLSL

#include "utils.glsl"
#include "inputs.glsl"

struct ModelTransform {
    mat4 modelMatrix;
    mat4 invModelMatrix;
};

mat3 quatToMat3(in vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    vec3 c0 = vec3(1.0 - 2.0*(y*y + z*z), 2.0*(x*y + z*w),       2.0*(x*z - y*w));
    vec3 c1 = vec3(2.0*(x*y - z*w),       1.0 - 2.0*(x*x + z*z), 2.0*(y*z + x*w));
    vec3 c2 = vec3(2.0*(x*z + y*w),       2.0*(y*z - x*w),       1.0 - 2.0*(x*x + y*y));
    return mat3(c0, c1, c2);
}

mat4 composeFromSample(in MotionSample s) {
    mat3 rot = quatToMat3(s.rotation);
    mat4 m = mat4(mat3(rot[0] * s.scale.x, rot[1] * s.scale.y, rot[2] * s.scale.z));
    m[3].xyz = s.translation;
    return m;
}

mat4 analyticInverseTRS(in MotionSample s) {
    mat3 invRot = transpose(quatToMat3(s.rotation));
    vec3 invScale = 1.0 / s.scale;
    mat3 invRS = mat3(invRot[0] * invScale, invRot[1] * invScale, invRot[2] * invScale);
    mat4 m = mat4(invRS);
    m[3].xyz = -(invRS * s.translation);
    return m;
}

ModelTransform sampleMotion(uint motionOffset) {
    MotionSample s = motionBuffer.samples[motionOffset];
    return ModelTransform(composeFromSample(s), analyticInverseTRS(s));
}

CameraPose sampleCameraPose(uint motionOffset) {
    ModelTransform mt = sampleMotion(motionOffset);
    vec3 dir = normalize(mat3(mt.modelMatrix) * vec3(0.0, 0.0, -1.0));
    vec3 right = normalize(cross(dir, vec3(0.0, 1.0, 0.0)));
    vec3 up = cross(right, dir);
    return CameraPose(mt.modelMatrix[3].xyz, dir, right, up);
}

#endif
