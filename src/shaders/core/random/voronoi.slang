import "basics.slang";

static const int VORONOI_CHECK_RADIUS = 2;
static const int VORONOI_CHECK_WIDTH = VORONOI_CHECK_RADIUS * 2 + 1;
struct VoronoiState {
    int2 cellIndex;
    float2 cellCenter;
    float distToCenter;
    float distToBorder;
};

float2 getCenter(in int2 cell, in float randomness, in RngState rng) {
    RngState localRng = initRngState(cell, 0);
    offsetRngState(localRng, rng);

    float2 offset = float2(rand(localRng), rand(localRng));
    offset = lerp(-float2(randomness), float2(randomness), offset);
    return float2(0.5) + offset;
}

float distToLine(float2 P, float2 a, float2 b) {
    float2 A = lerp(a, b, 0.5);
    float2 D = normalize(float2((a-b).y, (b-a).x));
    float d = length(A + dot(D, P - A) * D - P);
    return d;
}

VoronoiState voronoiNoise(in float2 p, in float randomness, inout RngState rng) {
    hashRngState(rng);

    VoronoiState state;

    int2 cell = int2(floor(p));
    float2 local = frac(p);

    state.distToCenter = 1e10;
    for (int i = 0; i < VORONOI_CHECK_WIDTH; i++) {
        for (int j = 0; j < VORONOI_CHECK_WIDTH; j++) {
            int dx = i - VORONOI_CHECK_RADIUS;
            int dy = j - VORONOI_CHECK_RADIUS;

            int2 index = cell + int2(dx, dy);
            float2 center = getCenter(index, randomness, rng) + float2(dx, dy);

            float dist = length(local - center);
            if (dist < state.distToCenter) {
                state.cellIndex = index;
                state.cellCenter = center;
                state.distToCenter = dist;
            }
        }
    }

    state.distToBorder = 1e10;
    for (int i = 0; i < VORONOI_CHECK_WIDTH; i++) {
        for (int j = 0; j < VORONOI_CHECK_WIDTH; j++) {
            int dx = i - VORONOI_CHECK_RADIUS;
            int dy = j - VORONOI_CHECK_RADIUS;
            int2 index = cell + int2(dx, dy);
            float2 center = getCenter(index, randomness, rng) + float2(dx, dy);

            state.distToBorder = min(
                state.distToBorder,
                distToLine(local, state.cellCenter, center)
            );
        }
    }

    return state;
}

// TODO: add a 3D version
