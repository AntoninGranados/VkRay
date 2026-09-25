#sky: version(1)

#param vec3 zenithColor = vec3(0.01, 0.01, 0.03): color
#param vec3 horizonColor = vec3(0.05, 0.05, 0.1): color

void main() {
    float t = clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
    result = mix(horizonColor, zenithColor, t);
}
