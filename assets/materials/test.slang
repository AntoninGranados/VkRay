import "script.slang";

#define NUMBER_W 5
#define NUMBER_H NUMBER_W

#define NUMBER_0 { \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_1 { \
    {0, 0, 1, 0, 0}, \
    {0, 1, 1, 0, 0}, \
    {1, 0, 1, 0, 0}, \
    {0, 0, 1, 0, 0}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_2 { \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 0}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_3 { \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {0, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_4 { \
    {1, 0, 0, 0, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {0, 0, 0, 0, 1} }

#define NUMBER_5 { \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 0}, \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_6 { \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 0}, \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_7 { \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {0, 0, 0, 1, 0}, \
    {0, 0, 1, 0, 0}, \
    {0, 0, 1, 0, 0} }

#define NUMBER_8 { \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_9 { \
    {1, 1, 1, 1, 1}, \
    {1, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1}, \
    {0, 0, 0, 0, 1}, \
    {1, 1, 1, 1, 1} }

#define NUMBER_EMPTY { \
    {0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0}, \
    {0, 0, 0, 0, 0} }

#define NUMBER_HEART { \
    {0, 1, 0, 1, 0}, \
    {1, 1, 1, 1, 1}, \
    {1, 1, 1, 1, 1}, \
    {0, 1, 1, 1, 0}, \
    {0, 0, 1, 0, 0} }

static const int NUMBERS[11][NUMBER_H][NUMBER_W] = {
    NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4, NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9, NUMBER_EMPTY
};

#define COLOR_COUNT 7
#define OVERLAY_COLOR_COUNT 3

struct Test : IMaterial {
    [Min(0)] float scale = 1.0;
    [Range(0, 0.1)] float feather = 0.01;
    [Range(0, 0.5)] float radius = 0.480;
    [Range(1, 10)] int padding = 4;
    bool coloredCheckerboard = false;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        const float3 COLORS[COLOR_COUNT] = {
            pow(float3(255,  85,  85) / 255, float3(2.2)),
            pow(float3(255, 184, 108) / 255, float3(2.2)),
            pow(float3(241, 250, 140) / 255, float3(2.2)),
            pow(float3( 80, 250, 123) / 255, float3(2.2)),
            pow(float3(139, 233, 253) / 255, float3(2.2)),
            pow(float3(189, 147, 249) / 255, float3(2.2)),
            pow(float3(255, 121, 198) / 255, float3(2.2))
        };
        const float3 OVERLAY_COLORS[OVERLAY_COLOR_COUNT] = { COLORS[0], COLORS[3], COLORS[5] };

        uv  /= scale;
        uv.x = uv.x - 1;
        uv.y = 1 - uv.y;
        float2 local = frac(uv);
        float2 circleLocal = frac(uv / 2) - float2(0.5);

        int2 idx = int2(floor(uv));
        int2 px;
        px.x = int(floor(local.x * (NUMBER_W + padding * 2)));
        px.y = int(floor(local.y * (NUMBER_H + padding * 2)));

        int colorIndex = int(mod(mod(float(idx.x - idx.y + 1), float(COLOR_COUNT)) + float(COLOR_COUNT), float(COLOR_COUNT)));
        float3 albedo = COLORS[colorIndex];

        float r = sqrt(circleLocal.x * circleLocal.x + circleLocal.y * circleLocal.y);
        if (r > radius - feather && r < radius + feather) {
            albedo = float3(1);
        }
        if (r > radius * 0.5 - feather && r < radius * 0.5 + feather) {
            albedo = float3(1);
        }

        if (
            px.x >= padding && px.x <= NUMBER_W + padding - 1 &&
            px.y >= padding && px.y <= NUMBER_H + padding - 1 &&
            // NUMBER_HEART[px.y-padding][px.x-padding] == 1
            NUMBERS[int(mod(idx.x + idx.y + 1, 10))][px.y-padding][px.x-padding] == 1
        ){
            albedo *= 0.1;
        } else if (
            abs(uv.x - round(uv.x * (NUMBER_W + 2 * padding)) / (NUMBER_W + 2 * padding)) < feather ||
            abs(uv.y - round(uv.y * (NUMBER_H + 2 * padding)) / (NUMBER_H + 2 * padding)) < feather
        ){
            albedo *= 0.5;
        }

        if (coloredCheckerboard) {
            int2 blockIdx = int2(floor(uv / 4));
            int i = int((mod(blockIdx.x, 2) + 2 * mod(blockIdx.y, 2)));
            if (i > 0) {
                albedo *= OVERLAY_COLORS[(i - 1) % OVERLAY_COLOR_COUNT];
            }
        }

        return Diffuse(albedo);
    }
}
