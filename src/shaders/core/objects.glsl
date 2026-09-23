#ifndef OBJECTS_GLSL
#define OBJECTS_GLSL

#include "utils.glsl"
#include "random/utils.glsl"
#include "inputs.glsl"
#include "motion.glsl"

Hit makeHit(in Ray ray, in Object obj, float t, vec2 uv, vec3 normal) {
    vec3 p = ray.origin + ray.dir * t;
    bool frontFace = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        frontFace = false;
    }
    return Hit(p, uv, normal, t, frontFace, obj, vec3(1.0));
}

// ================ UV ================
vec2 sphereUV(in vec3 n) {
    vec3 a = abs(n);
    if (a.x > a.y && a.x > a.z) return vec2(n.x < 0.0 ? n.z : -n.z, n.y) / a.x * 0.5 + 0.5;
    if (a.y > a.x && a.y > a.z) return vec2(n.x, n.y > 0.0 ? n.z : -n.z) / a.y * 0.5 + 0.5;
    return vec2(n.z > 0.0 ? n.x : -n.x, n.y) / a.z * 0.5 + 0.5;
}

vec2 aabbUV(in vec3 p, in vec3 n) {
    vec3 a = abs(n);
    if (a.x > a.y && a.x > a.z) return vec2(n.x < 0.0 ? p.z : -p.z, p.y) * 0.5 + 0.5;
    if (a.y > a.x && a.y > a.z) return vec2(p.x, n.y > 0.0 ? p.z : -p.z) * 0.5 + 0.5;
    return vec2(n.z > 0.0 ? p.x : -p.x, p.y) * 0.5 + 0.5;
}

// ================ NORMALS ================
vec3 sphereNormal(in vec3 p) {
    return normalize(p);
}

// ================ RAY INTERSECTION ================
Hit raySphereIntersection(in Ray ray) {
    vec3 p = -ray.origin;
    float a = dot(ray.dir, ray.dir);
    float b = dot(ray.dir, p);
    float c = dot(p, p) - 1.0;
    float delta = b * b - a * c;
    if (delta < 0.0) return NO_HIT;

    float sq = sqrt(delta);
    float t = (b - sq) / a;
    if (t < 0.0) t = (b + sq) / a;
    if (t < 0.0) return NO_HIT;

    vec3 hitP = ray.origin + ray.dir * t;
    vec3 normal = sphereNormal(hitP);
    return makeHit(ray, OBJECT_AABB, t, sphereUV(normal), normal);
}

Hit rayPlaneIntersection(in Ray ray) {
    if (abs(ray.dir.y) <= EPS) return NO_HIT;
    float t = -ray.origin.y / ray.dir.y;
    if (t < EPS) return NO_HIT;

    vec3 p = ray.origin + ray.dir * t;
    return makeHit(ray, OBJECT_AABB, t, vec2(p.x, p.z), vec3(0.0, 1.0, 0.0));
}

Hit rayQuadIntersection(in Ray ray, float tMax) {
    if (ray.dir.z >= -EPS) return NO_HIT; // back-face or parallel
    float t = -ray.origin.z / ray.dir.z;
    if (t < EPS || t >= tMax) return NO_HIT;

    vec3 p = ray.origin + ray.dir * t;
    if (abs(p.x) > 0.5 || abs(p.y) > 0.5) return NO_HIT;

    return makeHit(ray, OBJECT_AABB, t, p.xy + vec2(0.5), vec3(0.0, 0.0, 1.0));
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

    vec3 a = abs(normal);
    vec3 p = ray.origin + ray.dir * tHit;
    return makeHit(ray, OBJECT_AABB, tHit, aabbUV(p, normal), normal);
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

Hit rayMeshIntersection(in Ray ray, in Mesh mesh, bool anyHit, float tMax, inout Statistics stats) {
    if (mesh.bvhNodeCount == 0u) return NO_HIT;

    vec3 invDir = 1.0 / ray.dir;
    vec3 meshAabbMin = vec3(mesh.aabbMinX, mesh.aabbMinY, mesh.aabbMinZ);
    vec3 meshAabbMax = vec3(mesh.aabbMaxX, mesh.aabbMaxY, mesh.aabbMaxZ);
    stats.bvhChecks++;
    if (rayAabbTNear(ray, invDir, meshAabbMin, meshAabbMax) >= tMax) return NO_HIT;

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
                float tLocal = rayTriangleTNear(ray, v0, v1, v2, u, v);
                if (tLocal > 0.0 && tLocal < tClosest) {
                    if (anyHit) return makeHit(ray, OBJECT_AABB, tLocal, vec2(0), vec3(0.0, 1.0, 0.0));
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
                rayAabbTNear(ray, invDir,
                    vec3(node.children[0].minX, node.children[0].minY, node.children[0].minZ),
                    vec3(node.children[0].maxX, node.children[0].maxY, node.children[0].maxZ)),
                rayAabbTNear(ray, invDir,
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
    if (mesh.smoothShading == 1u) {
        vec3 n0 = vertexBuffer.vertices[bestI0].normal;
        vec3 n1 = vertexBuffer.vertices[bestI1].normal;
        vec3 n2 = vertexBuffer.vertices[bestI2].normal;
        normal = normalize((1.0 - bestU - bestV) * n0 + bestU * n1 + bestV * n2);
    } else {
        vec3 v0 = vertexBuffer.vertices[bestI0].position;
        vec3 v1 = vertexBuffer.vertices[bestI1].position;
        vec3 v2 = vertexBuffer.vertices[bestI2].position;
        normal = normalize(cross(v1 - v0, v2 - v0));
    }

    Hit meshHit = makeHit(ray, OBJECT_AABB, tClosest, vec2(0), normal);
    if (mesh.hasVertexColor == 1u) {
        vec3 c0 = vertexBuffer.vertices[bestI0].color;
        vec3 c1 = vertexBuffer.vertices[bestI1].color;
        vec3 c2 = vertexBuffer.vertices[bestI2].color;
        meshHit.vertexColor = (1.0 - bestU - bestV) * c0 + bestU * c1 + bestV * c2;
    }
    return meshHit;
}

// ================ SURFACE SAMPLING ================
SurfaceSample sampleSphereSurface(inout RngState rng) {
    SurfaceSample surfaceSample;
    surfaceSample.normal = normalize(randomInBall(rng));
    surfaceSample.p = surfaceSample.normal;
    return surfaceSample;
}

SurfaceSample sampleBoxSurface(in ModelTransform mt, in float area, inout RngState rng) {
    SurfaceSample surfaceSample;

    vec3 axisX = vec3(mt.modelMatrix[0]);
    vec3 axisY = vec3(mt.modelMatrix[1]);
    vec3 axisZ = vec3(mt.modelMatrix[2]);
    vec3 size = 2.0 * vec3(length(axisX), length(axisY), length(axisZ));
    vec3 pairArea = vec3(size.y * size.z, size.z * size.x, size.x * size.y);

    float r = rand(rng) * area;
    vec2 uv = vec2(rand(rng), rand(rng));

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

    return surfaceSample;
}

SurfaceSample sampleQuadSurface(inout RngState rng) {
    SurfaceSample surfaceSample;
    surfaceSample.p = vec3(rand(rng) - 0.5, rand(rng) - 0.5, 0.0);
    surfaceSample.normal = vec3(0.0, 0.0, 1.0);
    return surfaceSample;
}

SurfaceSample sampleMeshSurface(in Mesh mesh, inout RngState rng) {
    SurfaceSample surfaceSample;
    if (mesh.triangleCount == 0u) {
        surfaceSample.p = vec3(0.0);
        surfaceSample.normal = vec3(0.0);
        return surfaceSample;
    }

    uint tri = uint(rand(rng) * float(mesh.triangleCount));
    if (tri >= mesh.triangleCount) tri = mesh.triangleCount - 1u;
    uint base = mesh.indexOffset + tri * 3u;
    uint i0 = indexBuffer.indices[base + 0u];
    uint i1 = indexBuffer.indices[base + 1u];
    uint i2 = indexBuffer.indices[base + 2u];

    vec3 v0 = vertexBuffer.vertices[i0].position;
    vec3 v1 = vertexBuffer.vertices[i1].position;
    vec3 v2 = vertexBuffer.vertices[i2].position;

    float r1 = sqrt(rand(rng));
    float r2 = rand(rng);
    surfaceSample.p = v0 * (1.0 - r1) + v1 * (r1 * (1.0 - r2)) + v2 * (r1 * r2);
    surfaceSample.normal = normalize(cross(v1 - v0, v2 - v0));
    return surfaceSample;
}


#endif
