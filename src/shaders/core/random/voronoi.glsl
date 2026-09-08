#ifndef RANDOM_VORONOI_GLSL
#define RANDOM_VORONOI_GLSL

#include "basics.glsl"

#define VORONOI_CHECK_RADIUS 2
#define VORONOI_CHECK_WIDTH VORONOI_CHECK_RADIUS*2+1
struct VoronoiState {
    ivec2 cellIndex;
    vec2 cellCenter;
    float distToCenter;
    float distToBorder;
};

vec2 getCenter(in ivec2 cell, in float randomness, in RngState rng) {
    RngState localRng = initRngState(cell, 0);
    offsetRngState(localRng, rng);

    vec2 offset = vec2(rand(localRng), rand(localRng));
    offset = mix(-vec2(randomness), vec2(randomness), offset);
    return vec2(0.5) + offset;
}

float distToLine(vec2 P, vec2 a, vec2 b) {
    vec2 A = mix(a, b, 0.5);
    vec2 D = normalize(vec2((a-b).y, (b-a).x));
    float d = length(A + dot(D, P - A) * D - P);
    return d;
}

VoronoiState voronoiNoise(in vec2 p, in float randomness, inout RngState rng) {
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
            vec2 center = getCenter(index, randomness, rng) + vec2(dx, dy);

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
            vec2 center = getCenter(index, randomness, rng) + vec2(dx, dy);

            state.distToBorder = min(
                state.distToBorder,
                distToLine(local, state.cellCenter, center)
            );
        }
    }

    return state;
}

// TODO: add a 3D version

#endif
