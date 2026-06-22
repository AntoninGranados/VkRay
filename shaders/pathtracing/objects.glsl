#ifndef OBJECT_GLSL
#define OBJECT_GLSL

#include "utils.glsl"
#include "materials/materials.glsl"
#include "inputs.glsl"

Hit makeHit(in Ray ray, in Object obj, float t, vec3 normal) {
    vec3 p = ray.origin + ray.dir * t;
    bool frontFace = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        frontFace = false;
    }
    return Hit(p, normal, t, frontFace, obj);
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
    vec3 a = abs(localP);
    if (a.x > a.y && a.x > a.z) normal = vec3(sign(localP.x), 0, 0);
    else if (a.y > a.z) normal = vec3(0, sign(localP.y), 0);
    else normal = vec3(0, 0, sign(localP.z));
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
Hit raySphereIntersection(in Ray ray, in Object obj, in Sphere sphere, bool anyHit, float tMax, inout Statistics stats) {
    vec3 p = sphere.center - ray.origin;
    float dp = dot(ray.dir, p);
    float c = dot(p, p) - sphere.radius*sphere.radius;
    float delta = dp*dp - c;
    if (delta < 0) return NO_HIT;

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

Hit rayPlaneIntersection(in Ray ray, in Object obj, in Plane plane, bool anyHit, float tMax, inout Statistics stats) {
    float denom = dot(plane.normal, ray.dir);
    
    if (abs(denom) > EPS) {
        float t = dot(plane.point - ray.origin, plane.normal) / denom;
        if (t >= EPS) return makeHit(ray, obj, t, plane.normal);
    }
    return NO_HIT;
}

Hit rayAabbIntersection(in Ray ray, in vec3 aabbMin, in vec3 aabbMax, in bool computeNormal) {
    vec3 invDir = 1.0 / ray.dir;
    vec3 t0 = (aabbMin - ray.origin) * invDir;
    vec3 t1 = (aabbMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);

    if (tFar < max(tNear, EPS)) {
        return NO_HIT;
    }

    float tHit = tNear;
    bool useFar = false;
    if (tHit < EPS) {
        if (computeNormal) {
            tHit = tFar;
            useFar = true;
        } else {
            tHit = 0.0;
        }
    }

    vec3 normal = vec3(0.0);
    if (computeNormal) {
        if (!useFar) {
            if (tmin.x >= tmin.y && tmin.x >= tmin.z) normal = vec3(sign(ray.dir.x) < 0.0 ? 1.0 : -1.0, 0.0, 0.0);
            else if (tmin.y >= tmin.z) normal = vec3(0.0, sign(ray.dir.y) < 0.0 ? 1.0 : -1.0, 0.0);
            else normal = vec3(0.0, 0.0, sign(ray.dir.z) < 0.0 ? 1.0 : -1.0);
        } else {
            if (tmax.x <= tmax.y && tmax.x <= tmax.z) normal = vec3(sign(ray.dir.x) < 0.0 ? -1.0 : 1.0, 0.0, 0.0);
            else if (tmax.y <= tmax.z) normal = vec3(0.0, sign(ray.dir.y) < 0.0 ? -1.0 : 1.0, 0.0);
            else normal = vec3(0.0, 0.0, sign(ray.dir.z) < 0.0 ? -1.0 : 1.0);
        }
    }

    return makeHit(ray, OBJECT_AABB, tHit, normal);
}

Hit rayBoxIntersection(in Ray ray, in Object obj, in Box box, bool anyHit, float tMax, inout Statistics stats) {
    vec3 localOrigin = (box.invModelMatrix * vec4(ray.origin, 1.0)).xyz;
    vec3 localDir = (box.invModelMatrix * vec4(ray.dir, 0.0)).xyz;
    Ray localRay = Ray(localOrigin, localDir);

    Hit hit = rayAabbIntersection(localRay, vec3(-1.0), vec3(1.0), true);
    if (!foundIntersection(hit)) return hit;


    mat3 normalMat = mat3(transpose(box.invModelMatrix));
    vec3 localOutwardNormal = hit.frontFace ? hit.normal : -hit.normal;
    vec3 worldNormal = normalize(normalMat * localOutwardNormal);
    vec3 worldP = (box.modelMatrix * vec4(hit.p, 1.0)).xyz;
    float tWorld = dot(worldP - ray.origin, ray.dir);
    return makeHit(ray, obj, tWorld, worldNormal);
}

float rayAabbTNear(in Ray ray, in vec3 invDir, in vec3 aabbMin, in vec3 aabbMax) {
    vec3 t0 = (aabbMin - ray.origin) * invDir;
    vec3 t1 = (aabbMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);

    if (tFar < max(tNear, EPS)) {
        return INFINITY;
    }

    return tNear;
}

float rayTriangleTNear(in Ray ray, vec3 v0, vec3 v1, vec3 v2, out float u, out float v) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 pvec = cross(ray.dir, edge2);
    float det = dot(edge1, pvec);
    if (abs(det) < TRI_EPS) return -1.0;

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - v0;
    u = dot(tvec, pvec) * invDet;
    if (u < -TRI_EPS || u > 1.0 + TRI_EPS) return -1.0;

    vec3 qvec = cross(tvec, edge1);
    v = dot(ray.dir, qvec) * invDet;
    if (v < -TRI_EPS || u + v > 1.0 + TRI_EPS) return -1.0;

    float t = dot(edge2, qvec) * invDet;
    return (t >= TRI_EPS) ? t : -1.0;
}

Hit rayMeshIntersectionBvhDebug(in Ray ray, in Object obj, in Mesh mesh) {
    if (mesh.bvhNodeCount == 0u) return NO_HIT;

    vec3 localOrigin = (mesh.invModelMatrix * vec4(ray.origin, 1.0)).xyz;
    vec3 localDir = (mesh.invModelMatrix * vec4(ray.dir, 0.0)).xyz;
    Ray localRay = Ray(localOrigin, localDir);

    const int DEBUG_DEPTH = 8;
    uint stack[BVH_STACK_SIZE];
    int stackDepth[BVH_STACK_SIZE];
    int stackPtr = 0;

    stack[stackPtr] = mesh.bvhOffset;
    stackDepth[stackPtr] = 0;
    stackPtr++;

    float tClosest = INFINITY;
    vec3 bestNormal = vec3(0.0, 1.0, 0.0);
    bool foundHit = false;

    while (stackPtr > 0) {
        stackPtr--;
        uint nodeIdx = stack[stackPtr];
        int depth = stackDepth[stackPtr];
        BvhNode node = bvhBuffer.bvhNodes[nodeIdx];

        if (depth >= DEBUG_DEPTH || node.triangleCount > 0u) {
            vec3 aabbMin = vec3(node.children[0].minX, node.children[0].minY, node.children[0].minZ);
            vec3 aabbMax = vec3(node.children[0].maxX, node.children[0].maxY, node.children[0].maxZ);
            Hit nodeHit = rayAabbIntersection(localRay, aabbMin, aabbMax, true);
            if (foundIntersection(nodeHit) && nodeHit.t < tClosest) {
                tClosest = nodeHit.t;
                bestNormal = nodeHit.normal;
                foundHit = true;
            }
            continue;
        }

        if (stackPtr + 2 <= BVH_STACK_SIZE) {
            stack[stackPtr] = node.children[0].index;
            stackDepth[stackPtr] = depth + 1;
            stackPtr++;
            stack[stackPtr] = node.children[1].index;
            stackDepth[stackPtr] = depth + 1;
            stackPtr++;
        }
    }

    if (!foundHit) return NO_HIT;

    mat3 normalMat = mat3(transpose(mesh.invModelMatrix));
    vec3 normal = normalize(normalMat * bestNormal);
    return makeHit(ray, obj, tClosest, normal);
}

Hit rayMeshIntersection(in Ray ray, in Object obj, in Mesh mesh, bool anyHit, float tMax, inout Statistics stats) {
    // return rayMeshIntersectionBvhDebug(ray, obj, mesh);

    if (mesh.bvhNodeCount == 0u) return NO_HIT;

    vec3 localOrigin = (mesh.invModelMatrix * vec4(ray.origin, 1.0)).xyz;
    vec3 localDir    = (mesh.invModelMatrix * vec4(ray.dir, 0.0)).xyz;
    Ray localRay = Ray(localOrigin, localDir);

    vec3 invDir = 1.0 / localRay.dir;
    vec3 meshAabbMin = vec3(mesh.aabbMinX, mesh.aabbMinY, mesh.aabbMinZ);
    vec3 meshAabbMax = vec3(mesh.aabbMaxX, mesh.aabbMaxY, mesh.aabbMaxZ);
    stats.bvhChecks++;
    if (rayAabbTNear(localRay, invDir, meshAabbMin, meshAabbMax) >= tMax) return NO_HIT;

    float tClosest = tMax;
    bool foundHit = false;
    uint bestI0 = 0u;
    uint bestI1 = 0u;
    uint bestI2 = 0u;
    float bestU, bestV;

    uint stack[BVH_STACK_SIZE];
    int stackPtr = 0;
    stack[stackPtr++] = mesh.bvhOffset;

    while (stackPtr > 0) {
        uint nodeIdx = stack[--stackPtr];
        BvhNode node = bvhBuffer.bvhNodes[nodeIdx];

        if (node.triangleCount > 0u) {
            stats.triangleChecks += node.triangleCount;
            for (uint i = 0u; i < node.triangleCount; i++) {
                uint base = (node.firstTriangle + i) * 3u;
                uint i0 = indexBuffer.indices[base + 0u];
                uint i1 = indexBuffer.indices[base + 1u];
                uint i2 = indexBuffer.indices[base + 2u];

                vec3 v0 = vertexBuffer.vertices[i0].position;
                vec3 v1 = vertexBuffer.vertices[i1].position;
                vec3 v2 = vertexBuffer.vertices[i2].position;

                float u, v;
                float tLocal = rayTriangleTNear(localRay, v0, v1, v2, u, v);
                if (tLocal > 0.0 && tLocal < tClosest) {
                    if (anyHit) return makeHit(ray, obj, tLocal, vec3(0.0, 1.0, 0.0));
                    tClosest = tLocal;
                    foundHit = true;
                    bestI0 = i0;
                    bestI1 = i1;
                    bestI2 = i2;
                    bestU = u;
                    bestV = v;
                }
            }
        } else {
            if (stackPtr + 2 > BVH_STACK_SIZE) continue;

            stats.bvhChecks += 2;
            float t[2] = float[](
                rayAabbTNear(localRay, invDir,
                    vec3(node.children[0].minX, node.children[0].minY, node.children[0].minZ),
                    vec3(node.children[0].maxX, node.children[0].maxY, node.children[0].maxZ)),
                rayAabbTNear(localRay, invDir,
                    vec3(node.children[1].minX, node.children[1].minY, node.children[1].minZ),
                    vec3(node.children[1].maxX, node.children[1].maxY, node.children[1].maxZ))
            );

            if (t[0] > t[1]) {
                if (t[0] < tClosest) stack[stackPtr++] = node.children[0].index;
                if (t[1] < tClosest) stack[stackPtr++] = node.children[1].index;
            } else {
                if (t[1] < tClosest) stack[stackPtr++] = node.children[1].index;
                if (t[0] < tClosest) stack[stackPtr++] = node.children[0].index;
            }
        }
    }

    if (!foundHit) return NO_HIT;

    vec3 normal;
    vec3 v0 = vertexBuffer.vertices[bestI0].position;
    vec3 v1 = vertexBuffer.vertices[bestI1].position;
    vec3 v2 = vertexBuffer.vertices[bestI2].position;
    normal = normalize(cross(v1 - v0, v2 - v0));
    // vec3 n0 = vertexBuffer.vertices[bestI0].normal;
    // vec3 n1 = vertexBuffer.vertices[bestI1].normal;
    // vec3 n2 = vertexBuffer.vertices[bestI2].normal;
    // normal = normalize((1.0 - bestU - bestV) * n0 + bestU * n1 + bestV * n2);

    mat3 normalMat = mat3(transpose(mesh.invModelMatrix));
    vec3 worldNormal = normalize(normalMat * normal);
    vec3 localP = localOrigin + localDir * tClosest;
    vec3 worldP = (mesh.modelMatrix * vec4(localP, 1.0)).xyz;
    float tWorld = dot(worldP - ray.origin, ray.dir);

    return makeHit(ray, obj, tWorld, worldNormal);
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
