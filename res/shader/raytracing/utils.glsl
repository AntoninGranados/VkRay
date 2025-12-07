#ifndef UTILS_GLSL
#define UTILS_GLSL

#define EPS 1e-3
#define INFINITY abs(1.0/0.0)

#define Enum uint

// Needs to be here for the Hit struct
#define obj_None    Enum(0)
#define obj_Sphere  Enum(1)
#define obj_Plane   Enum(2)
#define obj_Box     Enum(3)
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
