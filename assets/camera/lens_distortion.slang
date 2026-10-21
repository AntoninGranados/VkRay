import "script.slang";

struct LensDistortion : ILens {
    [Range(-1.0, 1.0)] float k1 = 0.0;
    [Range(-1.0, 1.0)] float k2 = 0.0;

    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng) {
        float r2 = dot(ndcPos, ndcPos);
        float2 distorted = ndcPos * (1.0 + k1 * r2 + k2 * r2 * r2);

        float3 right = pose.right * U;
        float3 up = pose.up * V;
        float3 focalPoint = pose.eye + (distorted.x * right - distorted.y * up + pose.dir) * focalDistance;

        float3 offset = float3(0.0);
        if (lensRadius > 0.0) {
            float2 p = sampleLens(rng);
            offset = lensRadius * (pose.right * p.x + pose.up * p.y);
        }

        LensSample result;
        result.origin = pose.eye + offset;
        result.direction = focalPoint - result.origin;
        return result;
    }
}
