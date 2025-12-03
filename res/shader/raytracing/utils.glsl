#ifndef UTILS_GLSL
#define UTILS_GLSL

#define MAX_BOUNCE_DEPTH 10 // TODO: make it a uniform
#define SAMPLES_PER_PIXEL 1 // TODO: make it a uniform
#define EPS 1e-3
#define INFINITY abs(1.0/0.0)

#define Enum uint

struct Camera {
    vec3 pos;
    vec3 dir;
    vec3 up;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Hit {
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
    int idx;
    Enum type;
};

#endif
