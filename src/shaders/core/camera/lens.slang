import "../scene.slang";
import "../random/utils.slang";

[anyValueSize(4)]
interface ILens {
    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng);
}

struct DefaultLens : ILens {
    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng) {
        return LensSample(pose.eye, pose.dir);
    }
}

extension<T : ILens> Script<T> : ILens {
    LensSample evaluate(float2 ndcPos, CameraPose pose, float U, float V, float lensRadius, float focalDistance, inout RngState rng) {
        return loadParams<T>(base).evaluate(ndcPos, pose, U, V, lensRadius, focalDistance, rng);
    }
}
