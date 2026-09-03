#material: version(1)

#param int seed: min(0)
#param float scale = 1.0: min(0)
#param float jitter = 1.0: min(0), max(1)
#param vec3 borderColor: color
#param float borderSize = 0.01: min(0), max(0.5)

vec2 getCenter(ivec2 cell, in RngState rng) {
    RngState localRng = initRngState(cell, 0);
    offsetRngState(localRng, rng);
    
    vec2 offset = vec2(rand(localRng), rand(localRng));
    offset = mix(-vec2(jitter), vec2(jitter), offset);
    return vec2(0.5) + offset;
}

float distToLine(vec2 P, vec2 a, vec2 b) {
    vec2 A = mix(a, b, 0.5);
    vec2 D = normalize(vec2((a-b).y, (b-a).x));
    float d = length(A + dot(D, P - A) * D - P);
    return d;
}

#define VORONOI_CHECK_RADIUS 2
#define VORONOI_CHECK_WIDTH VORONOI_CHECK_RADIUS*2+1
struct VoronoiState {
    ivec2 cellIndex;
    vec2 cellCenter;
    float distToCenter;
    float distToBorder;
    float jitter;
};

VoronoiState sampleVoronoi(in vec2 p, in float jitter, inout RngState rng) {
    hashRngState(rng);

    VoronoiState state;

    ivec2 cell = ivec2(floor(p));
    vec2 local = fract(p);

    state.distToCenter = 1e10;
    for (int i = 0; i < VORONOI_CHECK_WIDTH; i++) {
        for (int j = 0; j < VORONOI_CHECK_WIDTH; j++) {
            int dx = i - VORONOI_CHECK_RADIUS;
            int dy = j - VORONOI_CHECK_RADIUS;

            ivec2 index = cell + ivec2(dx, dy);
            vec2 center = getCenter(index, rng) + vec2(dx, dy);

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
            ivec2 index = cell + ivec2(dx, dy);
            vec2 center = getCenter(index, rng) + vec2(dx, dy);

            state.distToBorder = min(
                state.distToBorder,
                distToLine(local, state.cellCenter, center)
            );
        }
    }

    return state;
}

void main() {
    vec2 local = uv / scale;
    
    rng = RngState(seed);
    VoronoiState state = sampleVoronoi(local, jitter, rng);
    
    ivec2 idx = state.cellIndex;
    offsetRngState(rng, initRngState(idx, 0));
    vec3 albedo = vec3(rand(rng), rand(rng), rand(rng));

    if (state.distToBorder < borderSize) albedo = borderColor;
    mat = Diffuse(albedo);
}
