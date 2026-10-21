// --------------- INPUTS ---------------
// float3 dir

// --------------- OUTPUTS ---------------
// float3 (return value)

import "script.slang";

struct DaySky : ISky {
    [Color] float3 zenithColor = float3(0.5, 0.7, 1.0);
    [Color] float3 horizonColor = float3(1.0, 1.0, 1.0);

    float3 evaluate(float3 dir) {
        float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
        return lerp(horizonColor, zenithColor, t);
    }
}
