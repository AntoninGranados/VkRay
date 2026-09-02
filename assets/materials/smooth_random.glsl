#material: version(1)

void main() {
    int octave = 10;
    float lacunarity = 2.0;
    float gain = 0.5;
    float r = (fractalNoise(pos, octave, lacunarity, gain) + 1.0) / 2.0;
    mat = Diffuse(vec3(r));
}
