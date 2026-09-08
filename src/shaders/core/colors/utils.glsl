#ifndef COLORS_UTILS_GLSL
#define COLORS_UTILS_GLSL

vec3 srgbToLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

vec3 viridis(float t) {
    t = clamp(t, 0.0, 1.0);

    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575,   1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213,  0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081,  14.17993336680509,   56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601,  -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535,   26.3124352495832);

    vec3 srgb = c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
    return srgbToLinear(srgb);
}

// Formulation from [https://en.wikipedia.org/wiki/HSL_and_HSV#Color_conversion_formulae]
vec3 hsv2rgb(vec3 hsv) {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;

    float C =s * v;
    float H = h / 60.0;
    float X = C * (1 - abs(mod(H, 2) - 1));

    vec3 rgb;
    if (0 <= H && H < 1) rgb = vec3(C, X, 0);
    else if (1 <= H && H < 2) rgb = vec3(X, C, 0);
    else if (2 <= H && H < 3) rgb = vec3(0, C, X);
    else if (3 <= H && H < 4) rgb = vec3(0, X, C);
    else if (4 <= H && H < 5) rgb = vec3(X, 0, C);
    else if (5 <= H && H < 6) rgb = vec3(C, 0, X);

    float m = v - C;
    return rgb + m;
}

vec3 rgb2hsv(vec3 rgb) {
    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    float v = max(r, max(g, b));
    float C = v - min(r, min(g, b));

    float s = v == 0 ? 0 : C / v;

    float h;
    if (C == 0) h = 0;
    else if (v == r) h = 60 * mod((g - b) / C, 6);
    else if (v == g) h = 60 * ((b - r) / C + 2);
    else if (v == b) h = 60 * ((r - g) / C + 4);

    return vec3(h, s, v);
}

#endif
