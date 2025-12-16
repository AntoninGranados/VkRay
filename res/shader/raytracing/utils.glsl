#ifndef UTILS_GLSL
#define UTILS_GLSL

#define EPS 1e-3
#define PI 3.14159265
#define INFINITY abs(1.0/0.0)

#define Enum int

// ============== MATERIAL ==============
#define mat_Lambertian    Enum(0)
#define mat_Metal         Enum(1)
#define mat_Dielectric    Enum(2)
#define mat_Emissive      Enum(3)
#define mat_Glossy        Enum(4)
#define mat_Checkerboard  Enum(5)

struct Material {
    Enum type;
    vec3 albedo;
    float payload[2];
};

#define MaterialHandle int

// ============== OBJECTS  ==============
#define obj_None    Enum(0)
#define obj_Sphere  Enum(1)
#define obj_Plane   Enum(2)
#define obj_Box     Enum(3)

#define ObjectHandle int

struct Object {
    Enum type;
    uint id;
};

#define OBJECT_NONE Object(obj_None, -1)

struct Sphere {
    vec3 center;
    float radius;
    MaterialHandle materialHandle;
};

struct Plane {
    vec3 point;
    vec3 normal;
    MaterialHandle materialHandle;
};

struct Box {
    vec3 cornerMin;
    vec3 cornerMax;
    MaterialHandle materialHandle;
};

// ============== PATH-TRACING  ==============
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

// ============== LIGHTS ==============
#define lightMode_Day    Enum(0)
#define lightMode_Sunset Enum(1)
#define lightMode_Night  Enum(2)
#define lightMode_Empty  Enum(3)

struct Light {
    ObjectHandle objectHandle;
    float area;
    float pdf;     // 1 / area
};

struct SurfaceSample {
    vec3 p;
    vec3 normal;
    float pdfA;
};

#endif
