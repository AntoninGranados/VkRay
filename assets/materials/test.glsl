#material: version(1)

#param float scale = 1.0: min(0)
#param float feather = 0.01: min(0), max(0.1)
#param float radius = 0.480: min(0), max(0.5)
#param int padding = 4: min(1), max(10)
#param bool coloredCheckerboard = false

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

#define NUMBER_HEART int[NUMBER_H][NUMBER_W]( \
    int[](0, 1, 0, 1, 0), \
    int[](1, 1, 1, 1, 1), \
    int[](1, 1, 1, 1, 1), \
    int[](0, 1, 1, 1, 0), \
    int[](0, 0, 1, 0, 0))

#define NUMBERS int[11][NUMBER_H][NUMBER_W]( \
    NUMBER_0, NUMBER_1, NUMBER_2, NUMBER_3, NUMBER_4, NUMBER_5, NUMBER_6, NUMBER_7, NUMBER_8, NUMBER_9, NUMBER_EMPTY)

#define COLOR_COUNT 7
#define OVERLAY_COLOR_COUNT 3
#define COLORS vec3[COLOR_COUNT]( \
    pow(vec3(255,  85,  85) / 255, vec3(2.2)), \
    pow(vec3(255, 184, 108) / 255, vec3(2.2)), \
    pow(vec3(241, 250, 140) / 255, vec3(2.2)), \
    pow(vec3( 80, 250, 123) / 255, vec3(2.2)), \
    pow(vec3(139, 233, 253) / 255, vec3(2.2)), \
    pow(vec3(189, 147, 249) / 255, vec3(2.2)), \
    pow(vec3(255, 121, 198) / 255, vec3(2.2)))

#define OVERLAY_COLORS vec3[OVERLAY_COLOR_COUNT](COLORS[0], COLORS[3], COLORS[5])

void test() {
    // this is a test
}

void main() {
    uv  /= scale;
    uv.x = uv.x - 1;
    uv.y = 1 - uv.y;
    vec2 local = fract(uv);
    vec2 circleLocal = fract(uv / 2) - vec2(0.5);

    ivec2 idx = ivec2(floor(uv));
    ivec2 px;
    px.x = int(floor(local.x * (NUMBER_W + padding * 2)));
    px.y = int(floor(local.y * (NUMBER_H + padding * 2)));
        
    int colorIndex = int(mod(mod(float(idx.x - idx.y + 1), float(COLOR_COUNT)) + float(COLOR_COUNT), float(COLOR_COUNT)));
    vec3 albedo = COLORS[colorIndex];

    float r = sqrt(circleLocal.x * circleLocal.x + circleLocal.y * circleLocal.y);
    if (r > radius - feather && r < radius + feather) {
        albedo = vec3(1);
    }
    if (r > radius * 0.5 - feather && r < radius * 0.5 + feather) {
        albedo = vec3(1);
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
        ivec2 blockIdx = ivec2(floor(uv / 4));
        int i = int((mod(blockIdx.x, 2) + 2 * mod(blockIdx.y, 2)));
        if (i > 0) {
            albedo *= OVERLAY_COLORS[(i - 1) % OVERLAY_COLOR_COUNT];
        }
    }

    mat = Diffuse(albedo);
}
