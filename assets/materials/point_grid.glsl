#material: version(1)

#param float scale = 1.0 : min(0)
#param vec3 albedo = vec3(1.0) : color
#param float radius = 0.08 : min(0)

void main() {
    float x = abs(uv.x - round(uv.x / scale) * scale);
    float y = abs(uv.y - round(uv.y / scale) * scale);
    if (x * x + y * y < radius * radius) {
        mat = Glossy(albedo, 0.1, 2.0);
    } else {
        mat = Diffuse(vec3(0.02));
    }
}
