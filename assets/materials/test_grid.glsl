#material: version(1)

#param vec3 albedo = vec3(1.0) : color, animatable
#param float scale = 1.0: min(0), animatable
#param float feather = 0.02: min(0), animatable
#param float darkening = 0.8: min(0), max(1), animatable

void main() {
    const float small_feather = feather * 0.5;

    if (abs(uv.x - round(uv.x / scale) * scale) < feather || abs(uv.y - round(uv.y / scale) * scale) < feather) {
        return Diffuse(vec3(0.0));
    } else if (int(round(uv.x / scale + 0.5) + round(uv.y / scale + 0.5) + 1) % 2 == 0) {
        if (abs(uv.x - round(uv.x / scale * 2) * scale / 2) < small_feather || abs(uv.y - round(uv.y / scale * 2) * scale / 2) < small_feather) {
            mat = Diffuse(vec3(0.0));
        } else {
            mat = Glossy(albedo, 0.1, 2.0);
        }
    } else {
        mat = Glossy(albedo * darkening, 0.1, 2.0);
    }
}
