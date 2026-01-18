#ifndef OBJECT_GLSL
#define OBJECT_GLSL

#include "utils.glsl"
#include "materials.glsl"
#include "inputs.glsl"

const float TRI_EPS = 1e-6;

Hit makeHit(in Ray ray, in Object obj, float t, vec3 normal) {
    vec3 p = ray.origin + ray.dir * t;
    bool front_face = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        front_face = false;
    }
    return Hit(p, normal, t, front_face, obj);
}

// ================ NORMALS ================
vec3 sphereNormal(in Sphere sphere, in vec3 p) {
    return normalize(p - sphere.center);
}

vec3 planeNormal(in Plane plane, in vec3 p) {
    return plane.normal;
}

vec3 boxNormal(in Box box, in vec3 p) {
    vec3 localP = (box.invModelMatrix * vec4(p, 1.0)).xyz;
    vec3 normal = vec3(0, 0, 0);
    for (int d = 0; d < 3; d++) {
        if (abs(localP[d] + 1.0) < EPS) {
            normal[d] = -1.0;
            break;
        }
        if (abs(localP[d] - 1.0) < EPS) {
            normal[d] = 1.0;
            break;
        }
    }
    mat3 normalMat = mat3(transpose(box.invModelMatrix));
    return normalize(normalMat * normal);
}

vec3 meshNormal(in Mesh mesh, in vec3 p) {
    vec3 bestNormal = vec3(0.0, 1.0, 0.0);
    float bestDist = INFINITY;

    vec3 localP = (mesh.invModelMatrix * vec4(p, 1.0)).xyz;

    for (uint i = 0; i < mesh.triangleCount; i++) {
        uint base = mesh.indexOffset + i * 3u;
        uint i0 = indexBuffer.indices[base + 0u];
        uint i1 = indexBuffer.indices[base + 1u];
        uint i2 = indexBuffer.indices[base + 2u];

        vec3 v0 = vertexBuffer.vertices[i0].position;
        vec3 v1 = vertexBuffer.vertices[i1].position;
        vec3 v2 = vertexBuffer.vertices[i2].position;

        vec3 n = normalize(cross(v1 - v0, v2 - v0));
        float dist = abs(dot(localP - v0, n));
        if (dist > 1e-2) continue;

        vec3 v0v1 = v1 - v0;
        vec3 v0v2 = v2 - v0;
        vec3 v0p = localP - v0;
        float d00 = dot(v0v1, v0v1);
        float d01 = dot(v0v1, v0v2);
        float d11 = dot(v0v2, v0v2);
        float d20 = dot(v0p, v0v1);
        float d21 = dot(v0p, v0v2);
        float denom = d00 * d11 - d01 * d01;
        if (abs(denom) < EPS) continue;

        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0 - v - w;
        if (u >= -EPS && v >= -EPS && w >= -EPS) {
            if (dist < bestDist) {
                bestDist = dist;
                mat3 normalMat = mat3(transpose(mesh.invModelMatrix));
                bestNormal = normalize(normalMat * n);
            }
        }
    }
    return bestNormal;
}

// ================ RAY INTERSECTION ================
Hit raySphereIntersection(in Ray ray, in Object obj, in Sphere sphere) {
    vec3 p = sphere.center - ray.origin;
    float dp = dot(ray.dir, p);
    float c = dot(p, p) - sphere.radius*sphere.radius;
    float delta = dp*dp - c;
    if (delta < 0)
        return Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE);

    float t1 = dp - sqrt(delta);
    if (t1 >= 0) {
        vec3 hitP = ray.origin + ray.dir * t1;
        return makeHit(ray, obj, t1, sphereNormal(sphere, hitP));
    }

    float t2 = dp + sqrt(delta);
    if (t2 >= 0) {
        vec3 hitP = ray.origin + ray.dir * t2;
        return makeHit(ray, obj, t2, sphereNormal(sphere, hitP));
    }
    
    return NO_HIT;
}

Hit rayPlaneIntersection(in Ray ray, in Object obj, in Plane plane) {
    float denom = dot(plane.normal, ray.dir);
    
    if (abs(denom) > EPS) {
        float t = dot(plane.point - ray.origin, plane.normal) / denom;
        if (t >= EPS) return makeHit(ray, obj, t, plane.normal);
    }
    return NO_HIT;
}

Hit rayAabbIntersection(in Ray ray, in vec3 aabbMin, in vec3 aabbMax) {
    vec3 safeDir = sign(ray.dir) * max(abs(ray.dir), vec3(EPS));
    vec3 invDir = 1.0 / safeDir;
    vec3 t0 = (aabbMin - ray.origin) * invDir;
    vec3 t1 = (aabbMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);

    if (tFar < max(tNear, EPS)) {
        return NO_HIT;
    }

    vec3 normal = vec3(0.0);
    if (tNear == tmin.x) normal = vec3(sign(ray.dir.x) < 0.0 ? 1.0 : -1.0, 0.0, 0.0);
    else if (tNear == tmin.y) normal = vec3(0.0, sign(ray.dir.y) < 0.0 ? 1.0 : -1.0, 0.0);
    else normal = vec3(0.0, 0.0, sign(ray.dir.z) < 0.0 ? 1.0 : -1.0);

    return makeHit(ray, OBJECT_AABB, tNear, normal);
}

Hit rayBoxIntersection(in Ray ray, in Object obj, in Box box) {
    vec3 localOrigin = (box.invModelMatrix * vec4(ray.origin, 1.0)).xyz;
    vec3 localDir = (box.invModelMatrix * vec4(ray.dir, 0.0)).xyz;
    Ray localRay = Ray(localOrigin, localDir);

    Hit hit = rayAabbIntersection(localRay, vec3(-1.0), vec3(1.0));
    if (!foundIntersection(hit)) return hit;

    mat3 normalMat = mat3(transpose(box.invModelMatrix));
    vec3 worldNormal = normalize(normalMat * hit.normal);
    return makeHit(ray, obj, hit.t, worldNormal);
}

float rayTriangleIntersection(in Ray ray, vec3 v0, vec3 v1, vec3 v2) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 pvec = cross(ray.dir, edge2);
    float det = dot(edge1, pvec);
    if (abs(det) < TRI_EPS) return -1.0;

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - v0;
    float u = dot(tvec, pvec) * invDet;
    if (u < -TRI_EPS || u > 1.0 + TRI_EPS) return -1.0;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.dir, qvec) * invDet;
    if (v < -TRI_EPS || u + v > 1.0 + TRI_EPS) return -1.0;

    float t = dot(edge2, qvec) * invDet;
    return (t >= TRI_EPS) ? t : -1.0;
}

Hit rayMeshIntersection(in Ray ray, in Object obj, in Mesh mesh) {
    vec3 localOrigin = (mesh.invModelMatrix * vec4(ray.origin, 1.0)).xyz;
    vec3 localDir = (mesh.invModelMatrix * vec4(ray.dir, 0.0)).xyz;
    Ray localRay = Ray(localOrigin, localDir);

    BvhNode bvhNode = bvhBuffer.bvhNodes[mesh.bvhOffset];

    Hit hit = rayAabbIntersection(localRay, bvhNode.aabbMin, bvhNode.aabbMax);
    if (!foundIntersection(hit)) return hit;

    float tClosest = INFINITY;
    bool foundHit = false;
    vec3 bestNormal = vec3(0.0, 1.0, 0.0);
    for (uint i = 0; i < mesh.triangleCount; i++) {
        uint base = mesh.indexOffset + i * 3u;
        uint i0 = indexBuffer.indices[base + 0u];
        uint i1 = indexBuffer.indices[base + 1u];
        uint i2 = indexBuffer.indices[base + 2u];

        vec3 v0 = vertexBuffer.vertices[i0].position;
        vec3 v1 = vertexBuffer.vertices[i1].position;
        vec3 v2 = vertexBuffer.vertices[i2].position;

        float tLocal = rayTriangleIntersection(localRay, v0, v1, v2);
        if (tLocal > 0.0 && tLocal < tClosest) {
            tClosest = tLocal;
            bestNormal = normalize(cross(v1 - v0, v2 - v0));
            foundHit = true;
        }
    }

    mat3 normalMat = mat3(transpose(mesh.invModelMatrix));
    vec3 normal = normalize(normalMat * bestNormal);
    return foundHit ? makeHit(ray, obj, tClosest, normal) : NO_HIT;
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

    vec3 axisX = vec3(box.modelMatrix[0]);
    vec3 axisY = vec3(box.modelMatrix[1]);
    vec3 axisZ = vec3(box.modelMatrix[2]);
    vec3 size = 2.0 * vec3(length(axisX), length(axisY), length(axisZ));
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

    if (axis == 0) {
        float x = side < 0.0 ? -1.0 : 1.0;
        surfaceSample.p = vec3(
            x,
            mix(-1.0, 1.0, uv.x),
            mix(-1.0, 1.0, uv.y)
        );
        surfaceSample.normal = vec3(side, 0.0, 0.0);
    } else if (axis == 1) {
        float y = side < 0.0 ? -1.0 : 1.0;
        surfaceSample.p = vec3(
            mix(-1.0, 1.0, uv.x),
            y,
            mix(-1.0, 1.0, uv.y)
        );
        surfaceSample.normal = vec3(0.0, side, 0.0);
    } else {
        float z = side < 0.0 ? -1.0 : 1.0;
        surfaceSample.p = vec3(
            mix(-1.0, 1.0, uv.x),
            mix(-1.0, 1.0, uv.y),
            z
        );
        surfaceSample.normal = vec3(0.0, 0.0, side);
    }

    surfaceSample.p = (box.modelMatrix * vec4(surfaceSample.p, 1.0)).xyz;
    mat3 normalMat = mat3(transpose(box.invModelMatrix));
    surfaceSample.normal = normalize(normalMat * surfaceSample.normal);
    return surfaceSample;
}

SurfaceSample sampleMeshSurface(in Mesh mesh, in float area, inout uint seed) {
    SurfaceSample surfaceSample;
    if (mesh.triangleCount == 0u) {
        surfaceSample.p = vec3(0.0);
        surfaceSample.normal = vec3(0.0);
        return surfaceSample;
    }

    uint tri = uint(rand(seed) * float(mesh.triangleCount));
    if (tri >= mesh.triangleCount) tri = mesh.triangleCount - 1u;
    uint base = mesh.indexOffset + tri * 3u;
    uint i0 = indexBuffer.indices[base + 0u];
    uint i1 = indexBuffer.indices[base + 1u];
    uint i2 = indexBuffer.indices[base + 2u];

    vec3 v0 = vertexBuffer.vertices[i0].position;
    vec3 v1 = vertexBuffer.vertices[i1].position;
    vec3 v2 = vertexBuffer.vertices[i2].position;

    float r1 = sqrt(rand(seed));
    float r2 = rand(seed);
    vec3 localP = v0 * (1.0 - r1) + v1 * (r1 * (1.0 - r2)) + v2 * (r1 * r2);
    vec3 worldP = (mesh.modelMatrix * vec4(localP, 1.0)).xyz;

    surfaceSample.p = worldP;
    mat3 normalMat = mat3(transpose(mesh.invModelMatrix));
    surfaceSample.normal = normalize(normalMat * normalize(cross(v1 - v0, v2 - v0)));
    return surfaceSample;
}


#endif
