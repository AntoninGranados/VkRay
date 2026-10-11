#compositing: version(1)

#param int resolution = 200: min(1)
#param float feather = 0.2: min(0), max(1)
#param float maxRadius = 1.3: min(0), max(2)

#param float4 angles = float4(105, 75, 90, 15): min(0), max(360)

float2 getIndex(in float2 uv, in float aspect) {
    float dx = 1 / float(resolution);
    uv.x *= aspect;
    return floor(uv / dx + 0.5);
}

float2 getCenter(in float aspect, in float2 index) {
    float dx = 1 / float(resolution);
    float2 center = index * dx;
    center.x /= aspect;
    return center;
}

float2 getLocal(in float2 uv, in float2 center, in float aspect) {
    float dx = 1 / float(resolution);
    float2 local = (uv - center) / dx * 2;
    local.x *= aspect;
    return local;
}

float2 rotate(in float2 p, in float aspect, in float c, in float s) {
    p.x *= aspect;
    p = mul(p, float2x2(c, -s, s, c));
    p.x /= aspect;
    return p;
}

float getValue(in float2 uv, in float aspect, in int2 texSize, in float angle, in int channel) {
    angle = radians(angle);
    float c = cos(angle);
    float s = sin(angle);

    float2 gridUV = rotate(uv, aspect, c, s);
    float2 index = getIndex(gridUV, aspect);

    float v = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            float2 center = getCenter(aspect, index + float2(i, j));
            float2 local = getLocal(gridUV, center, aspect);
            float3 color = sampleSource(int2(rotate(center, aspect, c, -s) * texSize), texSize).rgb;

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
    float2 uv = float2(pixelCoord) / float2(texSize);
    float aspect = float(texSize.x) / float(texSize.y);

    float c = getValue(uv, aspect, texSize, angles.x, 0);
    float m = getValue(uv, aspect, texSize, angles.y, 1);
    float y = getValue(uv, aspect, texSize, angles.z, 2);
    float k = getValue(uv, aspect, texSize, angles.w, 3);

    result = float4(cmyk2rgb(CMYK(c, m, y, k)), 1);
}

