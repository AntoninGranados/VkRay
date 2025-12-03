#ifndef OBJECT_GLSL
#define OBJECT_GLSL

#include "utils.glsl"
#include "materials.glsl"

struct Sphere {
    vec3 center;
    float radius;
    Material mat;
};

struct Plane {
    vec3 point;
    vec3 normal;
    Material mat;
};

#define obj_None    Enum(0x0000)
#define obj_Sphere  Enum(0x0001)
#define obj_Plane   Enum(0x0002)

float raySphereIntersection(in Ray ray, in Sphere sphere) {
    vec3 p = sphere.center - ray.origin;
    float dp = dot(ray.dir, p);
    float c = dot(p, p) - sphere.radius*sphere.radius;
    float delta = dp*dp - c;
    if (delta < 0)
        return -1;

    float t1 = dp - sqrt(delta);
    if (t1 >= 0) return t1;

    float t2 = dp + sqrt(delta);
    if (t2 >= 0) return t2;
    
    return -1;
}

float rayPlaneIntersection(in Ray ray, in Plane plane) {
    float denom = dot(plane.normal, ray.dir);
    if (abs(denom) > EPS) {
        float t = dot(plane.point - ray.origin, plane.normal) / denom;
        if (t >= EPS) return t;
    }
    return -1;
}

#endif
