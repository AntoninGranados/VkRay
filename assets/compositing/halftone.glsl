#compositing: version(1)

#param int resolution = 200: min(1)
#param float feather = 0.2: min(0), max(1)
#param float maxRadius = 1.3: min(0), max(2)

#param vec4 angles = vec4(105, 75, 90, 15): min(0), max(360)

vec2 getIndex(in vec2 uv, in float aspect) {
    float dx = 1 / float(resolution);
    uv.x *= aspect;
    return floor(uv / dx + 0.5);
}

vec2 getCenter(in float aspect, in vec2 index) {
    float dx = 1 / float(resolution);
    vec2 center = index * dx;
    center.x /= aspect;
    return center;
}

vec2 getLocal(in vec2 uv, in vec2 center, in float aspect) {
    float dx = 1 / float(resolution);
    vec2 local = (uv - center) / dx * 2;
    local.x *= aspect;
    return local;
}

vec2 rotate(in vec2 p, in float aspect, in float c, in float s) {
    p.x *= aspect;
    p = mat2(c, -s, s, c) * p;
    p.x /= aspect;
    return p;
}

float getValue(in vec2 uv, in float aspect, in ivec2 texSize, in float angle, in int channel) {
    angle = radians(angle);
    float c = cos(angle);
    float s = sin(angle);

    vec2 gridUV = rotate(uv, aspect, c, s);
    vec2 index = getIndex(gridUV, aspect);

    float v = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 center = getCenter(aspect, index + vec2(i, j));
            vec2 local = getLocal(gridUV, center, aspect);
            vec3 color = sampleSource(ivec2(rotate(center, aspect, c, -s) * texSize), texSize).rgb;

            float l;
            switch (channel) {
                case 0: l = rgb2cmyk(color).c; break;
                case 1: l = rgb2cmyk(color).m; break;
                case 2: l = rgb2cmyk(color).y; break;
                default: l = rgb2cmyk(color).k; break;
            }
            l = clamp(l, 0, 1);

            float d = length(local) / maxRadius;
            v = max(v, 1 - smoothstep(-feather, feather, d - l));
        }
    }

    return v;
}

void main() {
    vec2 uv = vec2(pixelCoord) / vec2(texSize);
    float aspect = float(texSize.x) / float(texSize.y);

    float c = getValue(uv, aspect, texSize, angles.x, 0);
    float m = getValue(uv, aspect, texSize, angles.y, 1);
    float y = getValue(uv, aspect, texSize, angles.z, 2);
    float k = getValue(uv, aspect, texSize, angles.w, 3);

    result = vec4(cmyk2rgb(CMYK(c, m, y, k)), 1);
}

