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

static const int NUMBERS[11][NUMBER_H][NUMBER_W] = {
    NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4, NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9, NUMBER_EMPTY
};

struct RandomParams {
    [Animatable] float lineTranslation = 0.1;
    [Range(0, 0.8)] float width = 0.2;
    [Range(0, 0.2)] float wiggle = 0.1;
}

struct BrickParams {
    [Color] float3 color = float3(0.9, 0.15, 0.05);
    [Range(0, 1)] float colorVariation = 0.1;
}

struct CementParams {
    [Color] float3 color = float3(0.5);
    [Range(0, 0.1)] float width = 0.08;
}

struct DirtParams {
    bool enabled = false;
    [Color] float3 color = float3(0.05, 0.15, 0);
    [Range(0, 1)] float height = 0.5;
    [Range(0, 0.5)] float falloff = 0.2;
}

struct BorderParams {
    bool enabled = false;
    [Color] float3 color = float3(0);
    [Range(0, 0.1)] float width = 0.02;
}

struct TagParams {
    bool enabled = false;
    [Color] float3 color = float3(0, 1, 0);
    [Range(0, 99)] int value = 42;
    [Range(0, 1)] float2 position = float2(0.5, 0.5);
    [Range(0, 360)] float angle = 0;
    [Range(0, 1)] float scale = 0.3;
}

struct Brick : IMaterial {
    [Min(0)] float scale = 0.25;
    [Range(0, 1)] float brickHeight = 0.4;

    RandomParams random;
    BrickParams brick;
    CementParams cement;
    DirtParams dirt;
    BorderParams border;
    TagParams tag;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        float2 uv = hit.uv;
        int2 idx;
        float2 local;
        float brickWidth;

        local = uv / scale;
        local.y /= brickHeight;

        rng = initRngState(uint2(local.y, 0), 0);
        local.x = local.x + (rand(rng) * 2 - 1) * random.lineTranslation + 0.5 * floor(local.y);


        // Random Width
        {
            idx = int2(floor(local));
            local = frac(local);

            rng = initRngState(idx, 0);
            float r = 1 - rand(rng) * random.width;

            RngState rngL = initRngState(int2(idx.x-1, idx.y), 0);
            float rL = 1 - rand(rngL) * random.width;
            float xL = (0.5 * r - 0.5 * rL) / (r + rL);

            RngState rngR = initRngState(int2(idx.x+1, idx.y), 0);
            float rR = 1 - rand(rngR) * random.width;
            float xR = (0.5 * r + 1.5 * rR) / (r + rR);

            float x;

            if (local.x < xL) {
                idx.x -= 1;

                RngState rngLL = initRngState(int2(idx.x-1, idx.y), 0);
                float rLL = 1.0 - rand(rngLL) * random.width;
                float xLL = (-0.5 * rL - 1.5 * rLL) / (rL + rLL);
                x = xLL;
                brickWidth = xL - xLL;

            } else if (local.x > xR) {
                idx.x += 1;

                RngState rngRR = initRngState(int2(idx.x+1, idx.y), 0);
                float rRR = 1 - rand(rngRR) * random.width;
                float xRR = (1.5 * rR + 2.5 * rRR) / (rR + rRR);
                x = xR;
                brickWidth = xRR - xR;

            } else {
                x = xL;
                brickWidth = xR - xL;
            }

            local.x = (local.x - x) / brickWidth;
        }

        RngState rngBrick = initRngState(idx, 0);
        HSV hsvBrick = rgb2hsv(brick.color);
        hsvBrick.h = mod(hsvBrick.h + brick.colorVariation * (rand(rngBrick)*2 - 1) * 60 + 360, 360);
        float3 albedo = hsv2rgb(hsvBrick);

        local += lerp(float2(-random.wiggle*0.5), float2(random.wiggle*0.5), float2(rand(rngBrick), rand(rngBrick)));

        float hCement = cement.width * 0.5;
        if (
            local.x < hCement / brickWidth || 1 - local.x < hCement / brickWidth ||
            local.y < hCement / brickHeight || 1 - local.y < hCement / brickHeight
        ) {
            albedo = cement.color;
        }

        if (tag.enabled) {
            bool tagged = false;

            float2 tagLocal = (uv - tag.position) / scale / tag.scale;
            float cAngle = cos(tag.angle / 180.0 * PI);
            float sAngle = sin(tag.angle / 180.0 * PI);
            tagLocal = float2(
                cAngle * tagLocal.x - sAngle * tagLocal.y,
                sAngle * tagLocal.x + cAngle * tagLocal.y
            );
            int2 pixelIdx = int2(round(tagLocal));
            if (abs(pixelIdx.x + NUMBER_W / 2 + 1) <= NUMBER_W/2 && abs(pixelIdx.y) <= NUMBER_H/2) {
                int2 localIdx = int2(
                    pixelIdx.x + NUMBER_W,
                    NUMBER_H - (pixelIdx.y + NUMBER_H/2) - 1
                );
                if (NUMBERS[tag.value / 10][localIdx.y][localIdx.x] == 1) tagged = true;
            }
            if (abs(pixelIdx.x - NUMBER_W / 2 - 1) <= NUMBER_W/2 && abs(pixelIdx.y) <= NUMBER_H/2) {
                int2 localIdx = int2(
                    pixelIdx.x - 1,
                    NUMBER_H - (pixelIdx.y + NUMBER_H/2) - 1
                );
                if (NUMBERS[tag.value % 10][localIdx.y][localIdx.x] == 1) tagged = true;
            }

            rng = initRngState(int2(0, 0), 0);
            float2 ditherLocal = (uv - float2(0.5)) / scale * 10.0;
            float theta = rand(rng) * 2 * PI;
            float cTheta = cos(theta);
            float sTheta = sin(theta);
            ditherLocal = float2(
                cTheta * ditherLocal.x - sTheta * ditherLocal.y,
                sTheta * ditherLocal.x + cTheta * ditherLocal.y
            );
            int2 ditherIdx = int2(round(ditherLocal));
            if (tagged) {
                if ((ditherIdx.x + ditherIdx.y + 1) % 2 == 0) albedo = tag.color;
                else albedo = float3(0);
            }
        }

        if (this.dirt.enabled) {
            RngState tempState = initRngState(0u);
            float dirt = perlinNoise(float2(uv.x, uv.y * 0.4) * 20, tempState).value;
            dirt = lerp(dirt, 1, smoothstep(this.dirt.height - this.dirt.falloff, this.dirt.height + this.dirt.falloff, lerp(this.dirt.falloff, 1 - this.dirt.falloff, uv.y)));

            dirt = smoothstep(0.4, 0.6, dirt);
            dirt = clamp(dirt, 0, 1);
            albedo = lerp(this.dirt.color, albedo, dirt);
        }

        if (this.border.enabled) {
            bool border = uv.x > 1.0 - this.border.width || uv.x < this.border.width || uv.y > 1.0 - this.border.width || uv.y < this.border.width;
            albedo = lerp(this.border.color, albedo, border ? 0 : 1);
        }

        return Diffuse(albedo);
    }
}
