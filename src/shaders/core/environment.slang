import "utils.slang";
import "../generated/sky_dispatch.slang";

float3 skyColor(int skyParamsBase, float3 dir) {
    if (skyParamsBase >= 0)
        return dispatchSky(skyParamsBase, dir);
    return float3(0.1);
}
