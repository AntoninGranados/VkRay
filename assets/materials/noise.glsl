// --------------- INPUTS ---------------
// vec3 pos
// vec2 uv
// vec3 normal
// vec3 wo
// RngState rng

// --------------- OUTPUTS ---------------
// vec3 new_normal = normal
// Material mat

#material: version(1)

#param int seed: min(0)
#param float scale = 0.1: min(0.001), max(1)
#param float radius = 0.5: min(0), max(0.5)
#param float falloff = 0.1: min(0), max(2)

void main() {
    RngState rng = RngState(seed);
    float r = perlinNoise(uv / scale, rng);
    float circle = smoothstep(radius-falloff*0.5, radius+falloff*0.5, length(uv - vec2(0.5)));
    r -= circle;
    if (abs(r) < 0.04) {
        mat = Diffuse(vec3(0.3, 0.0, 0.0));
    } else {
        mat = Principled(vec3(0.6, 0.2, 0.9), 0.2, 1.0, 1.0, r < 0 ? 0 : 1);
    }
}
