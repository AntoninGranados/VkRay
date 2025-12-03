#ifndef UTILS_GLSL
#define UTILS_GLSL

#define MAX_BOUNCE_DEPTH 10 // TODO: make it a uniform
#define SAMPLES_PER_PIXEL 1 // TODO: make it a uniform
#define EPS 1e-3
#define INFINITY abs(1.0/0.0)

#define Enum uint

// Needs to be here for the Hit struct
#define obj_None    Enum(0x0000)
#define obj_Sphere  Enum(0x0001)
#define obj_Plane   Enum(0x0002)
#define obj_Box     Enum(0x0003)
struct Object {
    Enum type;
    uint id;
};

#define OBJECT_NONE Object(obj_None, -1)

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
    Object object;
};
#define foundIntersection(h) ((h).object.type != obj_None)

#endif
