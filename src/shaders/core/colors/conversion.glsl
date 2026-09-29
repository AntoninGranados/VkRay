#ifndef COLORS_CONVERSION_GLSL
#define COLORS_CONVERSION_GLSL

struct HSV {
    float h, s, v;
};

struct CMYK {
    float c, m, y, k;
};

// Formulation from [https://en.wikipedia.org/wiki/HSL_and_HSV#Color_conversion_formulae]
vec3 hsv2rgb(HSV hsv) {
    float C = hsv.s * hsv.v;
    float H = hsv.h / 60.0;
    float X = C * (1 - abs(mod(H, 2) - 1));

    vec3 rgb;
    if (0 <= H && H < 1) rgb = vec3(C, X, 0);
    else if (1 <= H && H < 2) rgb = vec3(X, C, 0);
    else if (2 <= H && H < 3) rgb = vec3(0, C, X);
    else if (3 <= H && H < 4) rgb = vec3(0, X, C);
    else if (4 <= H && H < 5) rgb = vec3(X, 0, C);
    else if (5 <= H && H < 6) rgb = vec3(C, 0, X);

    float m = hsv.v - C;
    return rgb + m;
}

HSV rgb2hsv(vec3 rgb) {
    HSV hsv;

    hsv.v = max(rgb.r, max(rgb.g, rgb.b));
    float C = hsv.v - min(rgb.r, min(rgb.g, rgb.b));

    hsv.s = hsv.v == 0 ? 0 : C / hsv.v;

    float h;
    if (C == 0) hsv.h = 0;
    else if (hsv.v == rgb.r) hsv.h = 60 * mod((rgb.g - rgb.b) / C, 6);
    else if (hsv.v == rgb.g) hsv.h = 60 * ((rgb.b - rgb.r) / C + 2);
    else if (hsv.v == rgb.b) hsv.h = 60 * ((rgb.r - rgb.g) / C + 4);

    return hsv;
}

vec3 cmyk2rgb(CMYK cmyk) {
    vec3 rgb;

    rgb.r = (1 - cmyk.c) * (1 - cmyk.k);
    rgb.g = (1 - cmyk.m) * (1 - cmyk.k);
    rgb.b = (1 - cmyk.y) * (1 - cmyk.k);

    return rgb;
}

CMYK rgb2cmyk(vec3 rgb) {
    CMYK cmyk;

    cmyk.k = 1 - max(rgb.r, max(rgb.g, rgb.b));

    float f = 1 / (1 - cmyk.k);
    cmyk.c = (1 - rgb.r - cmyk.k) * f;
    cmyk.m = (1 - rgb.g - cmyk.k) * f;
    cmyk.y = (1 - rgb.b - cmyk.k) * f;

    return cmyk;
}

#endif
