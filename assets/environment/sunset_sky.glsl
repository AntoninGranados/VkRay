#sky: version(1)

#param vec3 zenithColor = vec3(0.2, 0.1, 0.4): color
#param vec3 horizonColor = vec3(1.0, 0.4, 0.2): color

void main() {
    float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
    result = mix(horizonColor, zenithColor, t);
}
