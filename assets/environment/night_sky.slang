import "script.slang";

struct NightSky : ISky {
    [Color] float3 zenithColor = float3(0.01, 0.01, 0.03);
    [Color] float3 horizonColor = float3(0.05, 0.05, 0.1);

    float3 evaluate(float3 dir) {
        float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
        return lerp(horizonColor, zenithColor, t);
    }
}
