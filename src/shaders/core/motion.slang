import "scene.slang";

struct ModelTransform {
    float4x4 modelMatrix;
    float4x4 invModelMatrix;
};

float3x3 quatToMat3(in float4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float3 c0 = float3(1.0 - 2.0*(y*y + z*z), 2.0*(x*y + z*w),       2.0*(x*z - y*w));
    float3 c1 = float3(2.0*(x*y - z*w),       1.0 - 2.0*(x*x + z*z), 2.0*(y*z + x*w));
    float3 c2 = float3(2.0*(x*z + y*w),       2.0*(y*z - x*w),       1.0 - 2.0*(x*x + y*y));
    return float3x3(c0, c1, c2);
}

float4 eulerToQuat(in float3 e) {
    float3 c = cos(e * 0.5);
    float3 s = sin(e * 0.5);
    return float4(
        s.x * c.y * c.z - c.x * s.y * s.z,
        c.x * s.y * c.z + s.x * c.y * s.z,
        c.x * c.y * s.z - s.x * s.y * c.z,
        c.x * c.y * c.z + s.x * s.y * s.z
    );
}

float4x4 composeFromSample(in MotionSample s) {
    float3x3 rot = quatToMat3(s.rotation);
    return float4x4(
        float4(rot[0] * s.scale.x, 0.0),
        float4(rot[1] * s.scale.y, 0.0),
        float4(rot[2] * s.scale.z, 0.0),
        float4(s.translation, 1.0)
    );
}

float4x4 analyticInverseTRS(in MotionSample s) {
    float3x3 invRot = transpose(quatToMat3(s.rotation));
    float3 invScale = 1.0 / s.scale;
    float3x3 invRS = float3x3(invRot[0] * invScale, invRot[1] * invScale, invRot[2] * invScale);
    return float4x4(
        float4(invRS[0], 0.0),
        float4(invRS[1], 0.0),
        float4(invRS[2], 0.0),
        float4(-mul(s.translation, invRS), 1.0)
    );
}

ModelTransform sampleMotion(uint motionOffset) {
    MotionSample s = scene.motion[motionOffset];
    return ModelTransform(composeFromSample(s), analyticInverseTRS(s));
}

CameraPose sampleCameraPose(uint motionOffset) {
    ModelTransform mt = sampleMotion(motionOffset);
    float3 dir = normalize(mul(float3(0.0, 0.0, -1.0), float3x3(mt.modelMatrix)));
    float3 right = normalize(cross(dir, float3(0.0, 1.0, 0.0)));
    float3 up = cross(right, dir);
    return CameraPose(mt.modelMatrix[3].xyz, dir, right, up);
}
