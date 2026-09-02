// --------------- INPUTS ---------------
// vec3 pos
// vec3 normal
// vec3 wo
// RngState rng

// --------------- OUTPUTS ---------------
// vec3 new_normal = normal
// Material mat

#material: version(1)

#param float scale = 0.25: min(0)
#param float brickHeight = 0.4: min(0), max(1), animatable

#param float randomLineTranslation = 0.1: animatable
#param float randomWidth = 0.2: min(0), max(0.8)
#param float randomWiggle = 0.1: min(0), max(0.2)

#param vec3 brickColor = vec3(0.9, 0.15, 0.05): color
#param float randomBrickColorVariation = 0.3: min(0), max(0.8)

#param vec3 cementColor = vec3(0.5): color
#param float cementWidth = 0.08: min(0), max(0.1)

#param bool hasDirt = false
#param vec3 dirtColor = vec3(0.05, 0.15, 0): color
#param float dirtHeight = 0.5: min(0), max(1)
#param float dirtFalloff = 0.2: min(0), max(0.5)

#param bool hasBorder = false
#param vec3 borderColor = vec3(0): color
#param float borderWidth = 0.02: min(0), max(0.1)

#param bool hasTag = false
#param vec3 tagColor = vec3(0, 1, 0): color
#param int tagValue = 42: min(0), max(99)
#param vec2 tagPosition = vec2(0.5, 0.5): min(0), max(1)
#param float tagAngle = 0: min(0), max(360)
#param float tagScale = 0.3: min(0), max(1)

#define NUMBER_W 5
#define NUMBER_H NUMBER_W

#define NUMBER_0 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_1 int[NUMBER_H][NUMBER_W]( \
    int[](0, 0, 1, 0, 0), \
    int[](0, 1, 1, 0, 0), \
    int[](1, 0, 1, 0, 0), \
    int[](0, 0, 1, 0, 0), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_2 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 0), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_3 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](0, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_4 int[NUMBER_H][NUMBER_W]( \
    int[](1, 0, 0, 0, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](0, 0, 0, 0, 1))

#define NUMBER_5 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 0), \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_6 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 0), \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_7 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](0, 0, 0, 1, 0), \
    int[](0, 0, 1, 0, 0), \
    int[](0, 0, 1, 0, 0))

#define NUMBER_8 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_9 int[NUMBER_H][NUMBER_W]( \
    int[](1, 1, 1, 1, 1), \
    int[](1, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1), \
    int[](0, 0, 0, 0, 1), \
    int[](1, 1, 1, 1, 1))

#define NUMBER_EMPTY int[NUMBER_H][NUMBER_W]( \
    int[](0, 0, 0, 0, 0), \
    int[](0, 0, 0, 0, 0), \
    int[](0, 0, 0, 0, 0), \
    int[](0, 0, 0, 0, 0), \
    int[](0, 0, 0, 0, 0))

#define NUMBERS int[11][NUMBER_H][NUMBER_W]( \
    NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4, NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9, NUMBER_EMPTY)

void main() {
    ivec2 idx;
    vec2 local;
    float brickWidth;

    local = uv / scale;
    local.y /= brickHeight;

    rng = initRngState(uvec2(local.y, 0), 0);
    local.x = local.x + (rand(rng) * 2 - 1) * randomLineTranslation + 0.5 * floor(local.y);


    // Random Width
    {
        idx = ivec2(floor(local));
        local = fract(local);
        
        rng = initRngState(idx, 0);
        float r = 1 - rand(rng) * randomWidth;

        RngState rngL = initRngState(ivec2(idx.x-1, idx.y), 0);
        float rL = 1 - rand(rngL) * randomWidth;
        float xL = (0.5 * r - 0.5 * rL) / (r + rL);

        RngState rngR = initRngState(ivec2(idx.x+1, idx.y), 0);
        float rR = 1 - rand(rngR) * randomWidth;
        float xR = (0.5 * r + 1.5 * rR) / (r + rR);

        float x;

        if (local.x < xL) {
            idx.x -= 1;

            RngState rngLL = initRngState(ivec2(idx.x-1, idx.y), 0);
            float rLL = 1.0 - rand(rngLL) * randomWidth;
            float xLL = (-0.5 * rL - 1.5 * rLL) / (rL + rLL);
            x = xLL;
            brickWidth = xL - xLL;

        } else if (local.x > xR) {
            idx.x += 1;

            RngState rngRR = initRngState(ivec2(idx.x+1, idx.y), 0);
            float rRR = 1 - rand(rngRR) * randomWidth;
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
    vec3 albedo = brickColor * (1 - randomBrickColorVariation * rand(rngBrick));

    local += mix(vec2(-randomWiggle*0.5), vec2(randomWiggle*0.5), vec2(rand(rngBrick), rand(rngBrick)));

    float hCement = cementWidth * 0.5;
    if (
        local.x < hCement / brickWidth || 1 - local.x < hCement / brickWidth ||
        local.y < hCement / brickHeight || 1 - local.y < hCement / brickHeight
    ) {
        albedo = cementColor;
    }

    if (hasTag) {
        bool tagged = false;

        vec2 tagLocal = (uv - tagPosition) / scale / tagScale;
        float cAngle = cos(tagAngle / 180.0 * PI);
        float sAngle = sin(tagAngle / 180.0 * PI);
        tagLocal = vec2(
            cAngle * tagLocal.x - sAngle * tagLocal.y,
            sAngle * tagLocal.x + cAngle * tagLocal.y
        );
        ivec2 pixelIdx = ivec2(round(tagLocal));
        if (abs(pixelIdx.x + NUMBER_W / 2 + 1) <= NUMBER_W/2 && abs(pixelIdx.y) <= NUMBER_H/2) {
            ivec2 localIdx = ivec2(
                pixelIdx.x + NUMBER_W,
                NUMBER_H - (pixelIdx.y + NUMBER_H/2) - 1
            );
            if (NUMBERS[tagValue / 10][localIdx.y][localIdx.x] == 1) tagged = true;
        }
        if (abs(pixelIdx.x - NUMBER_W / 2 - 1) <= NUMBER_W/2 && abs(pixelIdx.y) <= NUMBER_H/2) {
            ivec2 localIdx = ivec2(
                pixelIdx.x - 1,
                NUMBER_H - (pixelIdx.y + NUMBER_H/2) - 1
            );
            if (NUMBERS[tagValue % 10][localIdx.y][localIdx.x] == 1) tagged = true;
        }

        rng = initRngState(ivec2(0, 0), 0);
        vec2 ditherLocal = (uv - vec2(0.5)) / scale * 10.0;
        float theta = rand(rng) * 2 * PI;
        float cTheta = cos(theta);
        float sTheta = sin(theta);
        ditherLocal = vec2(
            cTheta * ditherLocal.x - sTheta * ditherLocal.y,
            sTheta * ditherLocal.x + cTheta * ditherLocal.y
        );
        ivec2 ditherIdx = ivec2(round(ditherLocal));
        if (tagged) {
            if ((ditherIdx.x + ditherIdx.y + 1) % 2 == 0) albedo = tagColor;
            else albedo = vec3(0);
        }
    }

    if (hasDirt) {
        float dirt = fractalNoise(vec3(uv.x, uv.y * 0.4, 0) * 20, 8, 2.0, 0.5);
        dirt = mix(dirt, 1, smoothstep(dirtHeight - dirtFalloff, dirtHeight + dirtFalloff, mix(dirtFalloff, 1 - dirtFalloff, uv.y)));
        
        dirt = smoothstep(0.4, 0.6, dirt);
        dirt = clamp(dirt, 0, 1);
        albedo = mix(dirtColor, albedo, dirt);
    }

    if (hasBorder) {
        bool border = uv.x > 1.0 - borderWidth || uv.x < borderWidth || uv.y > 1.0 - borderWidth || uv.y < borderWidth;
        albedo = mix(borderColor, albedo, border ? 0 : 1);
    }

    mat = Diffuse(albedo);
}
