#material: version(1)

#param float time = 0.0: min(0), max(1), animatable
#param float scale = 0.2: min(0)
#param vec3 segmentColor = vec3(1): color
#param float segmentThickness = 0.05: min(0), max(0.1)
#param float segmentLength = 0.45: min(0), max(0.5)
#param float segmentHead = 0.1: min(0), max(0.5)

// #define PERLIN_ANIMATION
#define SIMPLEX_ANIMATION

#define ANIMATION_SECTION_COUNT -1

#ifdef PERLIN_ANIMATION
#undef ANIMATION_SECTION_COUNT
#define ANIMATION_SECTION_COUNT 5
#endif

#ifdef SIMPLEX_ANIMATION
#undef ANIMATION_SECTION_COUNT
#define ANIMATION_SECTION_COUNT 6
#endif


float distToSegment(vec2 p, vec2 P1, vec2 P2) {
    vec2 v = P2 - P1;
    vec2 w = p - P1;
    float t = clamp(dot(w, v) / dot(v, v), 0.0, 1.0);
    vec2 proj = P1 + t * v;
    return length(p - proj);
}

bool drawSegment(in vec2 p, in vec2 P1, in vec2 P2, in float t, out vec3 color) {
    float dist = distToSegment(p, P1, P2);

    vec2 dir = normalize(P2 - P1);
    vec2 tan = vec2(dir.y, -dir.x);

    dist = min(dist, distToSegment(p, P2, P2 - dir*segmentHead*t + tan*segmentHead*0.6*t));
    dist = min(dist, distToSegment(p, P2, P2 - dir*segmentHead*t - tan*segmentHead*0.6*t));

    float th = segmentThickness * t;
    if (dist < th) {
        if (dist < 0.6 * th) color = segmentColor;
        else color = vec3(0);
        return true;
    }
    return false;
}

float map(float t, float mini, float maxi, float newMini, float newMaxi) {
    return (t - mini) / (maxi - mini) * (newMaxi - newMini) + newMini;
}

float T(in int sectionIdx) {
    float t0 = float(sectionIdx) / float(ANIMATION_SECTION_COUNT);
    float t1 = float(sectionIdx + 1) / float(ANIMATION_SECTION_COUNT);
    return smoothstep(0, 1, map(time, t0, t1, 0, 1));
}

struct PerlinAnimationState {
    vec2 nearestCell;
    vec2 localGradient;
    float g;
    float r;
};
PerlinAnimationState computePerlinState(in vec2 p) {
    PerlinAnimationState state;

    ivec2 p0 = ivec2(floor(p));
    vec2 pf = fract(p);

    float u = fade(pf.x);
    float v = fade(pf.y);

    float n[2], nx[2];

    RngState rng;
    float dNearest = 1e10;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            ivec2 idx = p0 + ivec2(x, y);
            rng = initRngState(idx, 0);
            vec2 grad = randomOnCircle(rng);

            float d = length(p - idx);
            if (d < dNearest) {
                dNearest = d;
                state.nearestCell = vec2(x, y);
                state.localGradient = grad;
            }

            n[x] = dot(grad, pf - vec2(x, y));
        }
        nx[y] = mix(n[0], n[1], u);
    }

    state.g = dot(state.localGradient, fract(p) - state.nearestCell);
    state.g = state.g * 0.5 + 0.5;

    state.r = mix(nx[0], nx[1], v);
    state.r *= PERLIN_NOISE_2D_NORM;
    state.r = state.r*0.5 + 0.5;

    return state;
}

struct SimplexAnimationState {
    vec2 nearestCell;
    vec2 nearestPoint;
    vec2 localGradient;
    float g;
    float r;
    vec2 skewOffset;
};
SimplexAnimationState computeSimplexState(in vec2 p) {
    SimplexAnimationState state;

    const float N = 2;
    const float F = (sqrt(N + 1) - 1) / N;
    const float G = (1 - 1 / sqrt(N + 1)) / N;

    state.skewOffset = vec2(p.x + p.y) * F;
    vec2 ps = p + state.skewOffset;
    vec2 idx = floor(ps);
    vec2 local = fract(ps);

    vec2 P0 = idx - vec2(idx.x + idx.y) * G;
    vec2 p0 = p - P0;

    vec2 corners[3];
    corners[0] = vec2(0);
    if (p0.x > p0.y) corners[1] = vec2(1, 0);
    else corners[1] = vec2(0, 1);
    corners[2] = vec2(1);

    float n[3];
    RngState rng;
    vec2 offset, grad;
    float dNearest = 1e10;
    for (int i = 0; i < 3; i++) {
        offset = p0 - (corners[i] - vec2(corners[i].x + corners[i].y) * G);

        rng = initRngState(ivec2(idx + corners[i]), 0);
        grad = randomOnCircle(rng);

        float d = dot(offset, offset);
        if (d < dNearest) {
            dNearest = d;
            state.nearestCell = corners[i];
            state.nearestPoint = idx + corners[i] - vec2(idx.x + idx.y + corners[i].x + corners[i].y) * G;
            state.localGradient = grad;
        }

        n[i] = dot(offset, grad);
        n[i] *= pow(max(0, SIMPLEX_R - dot(offset, offset)), 4);
    }

    state.g = dot(
        state.localGradient,
        p0 - (state.nearestCell - vec2(state.nearestCell.x + state.nearestCell.y) * G)
    );
    state.g = state.g * 0.5 + 0.5;

    state.r = (n[0] + n[1] + n[2]) * 50 * 0.5 + 0.5;
    return state;
}


#ifdef PERLIN_ANIMATION
ResolvedMaterial anim(in int sectionIdx, in vec2 p) {
    float t = T(sectionIdx);

    if (sectionIdx == 0) {
        vec2 local = fract(mix(p * scale, p, t));
        return Diffuse(vec3(local, 0.0));
    }

    PerlinAnimationState state = computePerlinState(p);

    float segmentL = 1.0;
    float segmentW = 1.0;
    if (sectionIdx == 1) {
        segmentL = smoothstep(0.0, 0.2, t);
        segmentW = pow(segmentL, 0.2);
    } else if (sectionIdx == 4) {
        segmentL = 1.0 - smoothstep(0.0, 0.2, t);
        segmentW = pow(segmentL, 0.2);
    }

    vec2 p0 = floor(p);
    vec2 P1 = p0 + state.nearestCell;
    vec2 P2 = P1 + state.localGradient * segmentLength * segmentL;

    vec3 color;
    if (!drawSegment(p, P1, P2, segmentW, color)) {
        vec3 base = vec3(fract(p), 0.0);
        vec3 g = viridis(state.g);
        vec3 r = viridis(state.r);

        if (sectionIdx == 1)
            color = base;
        else if (sectionIdx == 2)
            color = mix(base, g, t);
        else if (sectionIdx == 3)
            color = mix(g, r, t);
        else // sectionIdx == 4
            color = r;
    }

    return Diffuse(color);
}
#endif

#ifdef SIMPLEX_ANIMATION
ResolvedMaterial anim(in int sectionIdx, in vec2 p) {
    float t = T(sectionIdx);

    SimplexAnimationState state = computeSimplexState(p);

    if (sectionIdx == 0) {
        vec2 local = fract(mix(p * scale, p, t));
        return Diffuse(vec3(local, 0.0));
    } else if (sectionIdx == 1) {
        vec2 local = fract(p + mix(vec2(0), state.skewOffset, t));
        return Diffuse(vec3(local, 0.0));
    }

    float segmentL = 1.0;
    float segmentW = 1.0;
    if (sectionIdx == 2) {
        segmentL = smoothstep(0.0, 0.2, t);
        segmentW = pow(segmentL, 0.2);
    } else if (sectionIdx == 5) {
        segmentL = 1.0 - smoothstep(0.0, 0.2, t);
        segmentW = pow(segmentL, 0.2);
    }

    ivec2 p0 = ivec2(floor(p));
    vec2 P1 = state.nearestPoint;
    vec2 P2 = P1 + state.localGradient * segmentLength * segmentL;

    vec3 color;
    if (!drawSegment(p, P1, P2, segmentW, color)) {
        vec3 base = vec3(fract(p + state.skewOffset), 0.0);
        vec3 g = viridis(state.g);
        vec3 r = viridis(state.r);

        if (sectionIdx == 2)
            color = base;
        else if (sectionIdx == 3)
            color = mix(base, g, t);
        else if (sectionIdx == 4)
            color = mix(g, r, t);
        else // sectionIdx == 5
            color = r;
    }

    return Diffuse(color);
}
#endif

void main() {
    vec2 p = uv / scale;
    for (int i = 0; i < ANIMATION_SECTION_COUNT; i++) {
        float tEnd = float(i + 1) / float(ANIMATION_SECTION_COUNT);
        if (time <= tEnd) {
            mat = anim(i, p);
            break;
        }
    }
}
