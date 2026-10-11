#material: version(1)

#param float time = 0.0: min(0), max(1), animatable
#param float scale = 0.2: min(0)
#param float3 segmentColor = float3(1): color
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


float distToSegment(float2 p, float2 P1, float2 P2) {
    float2 v = P2 - P1;
    float2 w = p - P1;
    float t = clamp(dot(w, v) / dot(v, v), 0.0, 1.0);
    float2 proj = P1 + t * v;
    return length(p - proj);
}

bool drawSegment(in float2 p, in float2 P1, in float2 P2, in float t, out float3 color) {
    float dist = distToSegment(p, P1, P2);

    float2 dir = normalize(P2 - P1);
    float2 tan = float2(dir.y, -dir.x);

    dist = min(dist, distToSegment(p, P2, P2 - dir*segmentHead*t + tan*segmentHead*0.6*t));
    dist = min(dist, distToSegment(p, P2, P2 - dir*segmentHead*t - tan*segmentHead*0.6*t));

    float th = segmentThickness * t;
    if (dist < th) {
        if (dist < 0.6 * th) color = segmentColor;
        else color = float3(0);
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
    float2 nearestCell;
    float2 localGradient;
    float g;
    float r;
};
PerlinAnimationState computePerlinState(in float2 p) {
    PerlinAnimationState state;

    int2 p0 = int2(floor(p));
    float2 pf = frac(p);

    float u = fade(pf.x);
    float v = fade(pf.y);

    float n[2], nx[2];

    RngState rng;
    float dNearest = 1e10;
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            int2 idx = p0 + int2(x, y);
            rng = initRngState(idx, 0);
            float2 grad = randomOnCircle(rng);

            float d = length(p - idx);
            if (d < dNearest) {
                dNearest = d;
                state.nearestCell = float2(x, y);
                state.localGradient = grad;
            }

            n[x] = dot(grad, pf - float2(x, y));
        }
        nx[y] = lerp(n[0], n[1], u);
    }

    state.g = dot(state.localGradient, frac(p) - state.nearestCell);
    state.g = state.g * 0.5 + 0.5;

    state.r = lerp(nx[0], nx[1], v);
    state.r *= PERLIN_NOISE_2D_NORM;
    state.r = state.r*0.5 + 0.5;

    return state;
}

struct SimplexAnimationState {
    float2 nearestCell;
    float2 nearestPoint;
    float2 localGradient;
    float g;
    float r;
    float2 skewOffset;
};
SimplexAnimationState computeSimplexState(in float2 p) {
    SimplexAnimationState state;

    const float N = 2;
    const float F = (sqrt(N + 1) - 1) / N;
    const float G = (1 - 1 / sqrt(N + 1)) / N;

    state.skewOffset = float2(p.x + p.y) * F;
    float2 ps = p + state.skewOffset;
    float2 idx = floor(ps);
    float2 local = frac(ps);

    float2 P0 = idx - float2(idx.x + idx.y) * G;
    float2 p0 = p - P0;

    float2 corners[3];
    corners[0] = float2(0);
    if (p0.x > p0.y) corners[1] = float2(1, 0);
    else corners[1] = float2(0, 1);
    corners[2] = float2(1);

    float n[3];
    RngState rng;
    float2 offset, grad;
    float dNearest = 1e10;
    for (int i = 0; i < 3; i++) {
        offset = p0 - (corners[i] - float2(corners[i].x + corners[i].y) * G);

        rng = initRngState(int2(idx + corners[i]), 0);
        grad = randomOnCircle(rng);

        float d = dot(offset, offset);
        if (d < dNearest) {
            dNearest = d;
            state.nearestCell = corners[i];
            state.nearestPoint = idx + corners[i] - float2(idx.x + idx.y + corners[i].x + corners[i].y) * G;
            state.localGradient = grad;
        }

        n[i] = dot(offset, grad);
        n[i] *= pow(max(0, SIMPLEX_R - dot(offset, offset)), 4);
    }

    state.g = dot(
        state.localGradient,
        p0 - (state.nearestCell - float2(state.nearestCell.x + state.nearestCell.y) * G)
    );
    state.g = state.g * 0.5 + 0.5;

    state.r = (n[0] + n[1] + n[2]) * 50 * 0.5 + 0.5;
    return state;
}


#ifdef PERLIN_ANIMATION
ResolvedMaterial anim(in int sectionIdx, in float2 p) {
    float t = T(sectionIdx);

    if (sectionIdx == 0) {
        float2 local = frac(lerp(p * scale, p, t));
        return Diffuse(float3(local, 0.0));
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

    float2 p0 = floor(p);
    float2 P1 = p0 + state.nearestCell;
    float2 P2 = P1 + state.localGradient * segmentLength * segmentL;

    float3 color;
    if (!drawSegment(p, P1, P2, segmentW, color)) {
        float3 base = float3(frac(p), 0.0);
        float3 g = viridis(state.g);
        float3 r = viridis(state.r);

        if (sectionIdx == 1)
            color = base;
        else if (sectionIdx == 2)
            color = lerp(base, g, t);
        else if (sectionIdx == 3)
            color = lerp(g, r, t);
        else // sectionIdx == 4
            color = r;
    }

    return Diffuse(color);
}
#endif

#ifdef SIMPLEX_ANIMATION
ResolvedMaterial anim(in int sectionIdx, in float2 p) {
    float t = T(sectionIdx);

    SimplexAnimationState state = computeSimplexState(p);

    if (sectionIdx == 0) {
        float2 local = frac(lerp(p * scale, p, t));
        return Diffuse(float3(local, 0.0));
    } else if (sectionIdx == 1) {
        float2 local = frac(p + lerp(float2(0), state.skewOffset, t));
        return Diffuse(float3(local, 0.0));
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

    int2 p0 = int2(floor(p));
    float2 P1 = state.nearestPoint;
    float2 P2 = P1 + state.localGradient * segmentLength * segmentL;

    float3 color;
    if (!drawSegment(p, P1, P2, segmentW, color)) {
        float3 base = float3(frac(p + state.skewOffset), 0.0);
        float3 g = viridis(state.g);
        float3 r = viridis(state.r);

        if (sectionIdx == 2)
            color = base;
        else if (sectionIdx == 3)
            color = lerp(base, g, t);
        else if (sectionIdx == 4)
            color = lerp(g, r, t);
        else // sectionIdx == 5
            color = r;
    }

    return Diffuse(color);
}
#endif

void main() {
    float2 p = uv / scale;
    for (int i = 0; i < ANIMATION_SECTION_COUNT; i++) {
        float tEnd = float(i + 1) / float(ANIMATION_SECTION_COUNT);
        if (time <= tEnd) {
            mat = anim(i, p);
            break;
        }
    }
}
