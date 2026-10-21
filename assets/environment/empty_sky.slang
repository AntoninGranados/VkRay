import "script.slang";

struct EmptySky : ISky {
    [Color] float3 background = float3(0);

    float3 evaluate(float3 dir) {
        return background;
    }
}
