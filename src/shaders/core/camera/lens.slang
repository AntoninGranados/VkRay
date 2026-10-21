import "../scene.slang";
import "../random/utils.slang";

[anyValueSize(4)]
interface ILens {
    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng);
}

struct ScriptLens<T : ILens> : ILens {
    uint base;

    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng) {
        return loadParams<T>(base).evaluate(ndcPos, pose, U, V, lensRadius, focalDistance, rng);
    }
}
