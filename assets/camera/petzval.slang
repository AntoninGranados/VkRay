import "script.slang";

struct Petzval : ILens {
    [Range(0.0, 4.0)] float astigmatism = 1.0;

    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng) {
        float3 right = pose.right * U;
        float3 up = pose.up * V;
        float3 focalPoint = pose.eye + (ndcPos.x * right - ndcPos.y * up + pose.dir) * focalDistance;

        float3 offset = float3(0.0);
        if (lensRadius > 0.0) {
            float2 fieldPos = float2(ndcPos.x * U, -ndcPos.y * V);
            float fieldHeight = length(fieldPos);
            float2 radialDir = fieldHeight > 1e-6 ? fieldPos / fieldHeight : float2(1.0, 0.0);
            float2 tangentialDir = float2(-radialDir.y, radialDir.x);

            float2 p = sampleLens(rng);
            float2 pLocal = float2(dot(p, radialDir), dot(p, tangentialDir));
            pLocal.y *= 1.0 + astigmatism * fieldHeight * fieldHeight;
            float2 pWorld = pLocal.x * radialDir + pLocal.y * tangentialDir;

            offset = lensRadius * (pose.right * pWorld.x + pose.up * pWorld.y);
        }

        LensSample result;
        result.origin = pose.eye + offset;
        result.direction = focalPoint - result.origin;
        return result;
    }
}
