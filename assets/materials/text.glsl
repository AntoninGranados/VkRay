#material: version(1)

#param int character = 0: min(0)
#param int padding = 0: min(0), max(4)

#define CHAR_WIDTH 5
#define CHAR_HEIGHT CHAR_WIDTH

#define CHAR_0 0x1f9d73f
#define CHAR_1 0x046509f
#define CHAR_2 0x1f0fe1f
#define CHAR_3 0x1f0bc3f
#define CHAR_4 0x118fc21
#define CHAR_5 0x1f87c3f
#define CHAR_6 0x1f87e3f
#define CHAR_7 0x1f08884
#define CHAR_8 0x1f8fe3f
#define CHAR_9 0x1f8fc3f

#define CHAR_A 0x1f8fe31
#define CHAR_B 0x1e8fa3e
#define CHAR_C 0x1f8421f
#define CHAR_D 0x1e8c63e
#define CHAR_E 0x1f87a1f
#define CHAR_F 0x1f87a10
#define CHAR_G 0x1f84e3f
#define CHAR_H 0x118fe31
#define CHAR_I 0x1f2109f
#define CHAR_J 0x1f10a5c
#define CHAR_K 0x12a6292
#define CHAR_L 0x108421f
#define CHAR_M 0x11dd631
#define CHAR_N 0x11cd671
#define CHAR_O 0x1f8c63f
#define CHAR_P 0x1f8fe10
#define CHAR_Q 0x1f8c67f
#define CHAR_R 0x1f8fe51
#define CHAR_S 0x0f8383e
#define CHAR_T 0x1f21084
#define CHAR_U 0x118c63f
#define CHAR_V 0x118a944
#define CHAR_W 0x118c6aa
#define CHAR_X 0x1151151
#define CHAR_Y 0x118a884
#define CHAR_Z 0x1f1111f

#define CHAR_SPACE 0x000000
#define CHAR_EMPTY 0xe89804

#define CHARACTER_COUNT 10 + 2
#define CHAR_NUM_START 0
#define CHAR_ALPHA_START 9
#define CHARACTERS int[]( \
    CHAR_0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9, \
    CHAR_A, CHAR_B, CHAR_C, CHAR_D, CHAR_E, CHAR_F, CHAR_G, CHAR_H, CHAR_I, CHAR_J, CHAR_K, CHAR_L, CHAR_M, CHAR_N, CHAR_O, CHAR_P, CHAR_Q, CHAR_R, CHAR_S, CHAR_T, CHAR_U, CHAR_V, CHAR_W, CHAR_X, CHAR_Y, CHAR_Z, \
    CHAR_SPACE, CHAR_EMPTY \
)

void main() {
    vec2 local = fract(uv);

    ivec2 idx = ivec2(floor(uv));
    ivec2 px;
    px.x = int(floor(local.x * (CHAR_WIDTH + padding * 2)));
    px.y = int(floor(local.y * (CHAR_HEIGHT + padding * 2)));

    vec3 albedo = vec3(1);

    if (
        px.x >= padding && px.x <= CHAR_WIDTH + padding - 1 &&
        px.y >= padding && px.y <= CHAR_HEIGHT + padding - 1
    ){
        uint x = CHAR_WIDTH - 1 - uint(px.x - padding);
        uint y = uint(px.y - padding);
        uint loc = uint(pow(2, y*5 + x));
        uint val = (CHARACTERS[character] / loc) % 2;
        if (val == 1u) albedo *= 0.1;
    }

    mat = Diffuse(albedo);
}

