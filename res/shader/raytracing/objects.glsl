#ifndef OBJECT_GLSL
#define OBJECT_GLSL

#include "utils.glsl"
#include "materials.glsl"

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

// ================ SURFACE SAMPLING ================
SurfaceSample sampleSphereSurface(in Sphere sphere, in float area, inout uint seed) {
    SurfaceSample surfaceSample;
    
    vec3 onLightDir = normalize(randomInSphere(seed));
    surfaceSample.p = sphere.center + onLightDir * sphere.radius;

    surfaceSample.normal = (surfaceSample.p - sphere.center) / sphere.radius;
    return surfaceSample;
}

SurfaceSample sampleBoxSurface(in Box box, in float area, inout uint seed) {
    SurfaceSample surfaceSample;

    vec3 size = box.cornerMax - box.cornerMin;
    vec3 pairArea = vec3(size.y * size.z, size.z * size.x, size.x * size.y);

    float r = rand(seed) * area;
    vec2 uv = vec2(rand(seed), rand(seed));

    int axis;
    float side;
    float range = 2.0 * pairArea.x;
    if (r < range) {
        axis = 0;
        side = (r < pairArea.x) ? -1.0 : 1.0;
    } else {
        r -= range;
        range = 2.0 * pairArea.y;
        if (r < range) {
            axis = 1;
            side = (r < pairArea.y) ? -1.0 : 1.0;
        } else {
            r -= range;
            axis = 2;
            side = (r < pairArea.z) ? -1.0 : 1.0;
        }
    }

    vec3 cornerMin = box.cornerMin;
    vec3 cornerMax = box.cornerMax;
    if (axis == 0) {
        float x = side < 0.0 ? cornerMin.x : cornerMax.x;
        surfaceSample.p = vec3(
            x,
            mix(cornerMin.y, cornerMax.y, uv.x),
            mix(cornerMin.z, cornerMax.z, uv.y)
        );
        surfaceSample.normal = vec3(side, 0.0, 0.0);
    } else if (axis == 1) {
        float y = side < 0.0 ? cornerMin.y : cornerMax.y;
        surfaceSample.p = vec3(
            mix(cornerMin.x, cornerMax.x, uv.x),
            y,
            mix(cornerMin.z, cornerMax.z, uv.y)
        );
        surfaceSample.normal = vec3(0.0, side, 0.0);
    } else {
        float z = side < 0.0 ? cornerMin.z : cornerMax.z;
        surfaceSample.p = vec3(
            mix(cornerMin.x, cornerMax.x, uv.x),
            mix(cornerMin.y, cornerMax.y, uv.y),
            z
        );
        surfaceSample.normal = vec3(0.0, 0.0, side);
    }

    return surfaceSample;
}

#endif
