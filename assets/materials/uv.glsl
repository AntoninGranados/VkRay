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

void main() {
    mat = Diffuse(clamp(vec3(uv.x, uv.y, 0.0), vec3(0), vec3(1)));
}
