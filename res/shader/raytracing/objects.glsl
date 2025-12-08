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

struct Box {
    vec3 cornerMin;
    vec3 cornerMax;
    Material mat;
};

// ================ RAY INTERSECTION ================
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

float rayBoxIntersection(in Ray ray, in Box box) {
    vec3 safeDir = sign(ray.dir) * max(abs(ray.dir), vec3(EPS));
    vec3 invDir = 1.0 / safeDir;
    bvec3 s = lessThan(ray.dir, vec3(0.0));

    float tmin = ( (s.x ? box.cornerMax.x : box.cornerMin.x) - ray.origin.x) * invDir.x;
    float tmax = ( (!s.x ? box.cornerMax.x : box.cornerMin.x) - ray.origin.x) * invDir.x;

    float tymin = ( (s.y ? box.cornerMax.y : box.cornerMin.y) - ray.origin.y) * invDir.y;
    float tymax = ( (!s.y ? box.cornerMax.y : box.cornerMin.y) - ray.origin.y) * invDir.y;

    tmin = max(tmin, tymin);
    tmax = min(tmax, tymax);

    float tzmin = ( (s.z ? box.cornerMax.z : box.cornerMin.z) - ray.origin.z) * invDir.z;
    float tzmax = ( (!s.z ? box.cornerMax.z : box.cornerMin.z) - ray.origin.z) * invDir.z;

    tmin = max(tmin, tzmin);
    tmax = min(tmax, tzmax);

    if (tmax >= max(tmin, EPS))
        return (tmin >= EPS) ? tmin : tmax;
    return -1.0;
}

// ================ NORMALS ================
vec3 sphereNormal(in Sphere sphere, in vec3 p) {
    return normalize(p - sphere.center);
}

vec3 planeNormal(in Plane plane, in vec3 p) {
    return plane.normal;
}

// TODO: optimize this !!
vec3 boxNormal(in Box box, in vec3 p) {
    vec3 normal = vec3(0, 0, 0);
    for (int d = 0; d < 3; d++) {
        if (abs(p[d] - box.cornerMin[d]) < EPS) {
            normal[d] = 1;
            break;
        }
        if (abs(p[d] - box.cornerMax[d]) < EPS) {
            normal[d] = -1;
            break;
        }
    }
    return normal;
}

#endif
