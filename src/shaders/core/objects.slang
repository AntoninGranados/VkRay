import "scene.slang";
import "random/utils.slang";
import "motion.slang";

Hit makeHit(in Ray ray, in Object obj, float t, float2 uv, float3 normal) {
    float3 p = ray.origin + ray.dir * t;
    bool frontFace = true;
    if (dot(ray.dir, normal) > 0.0) {
        normal = -normal;
        frontFace = false;
    }
    return Hit(p, uv, normal, t, frontFace, obj, float3(1.0));
}

// ================ UV ================
float2 sphereUV(in float3 n) {
    float3 a = abs(n);
    if (a.x > a.y && a.x > a.z) return float2(n.x < 0.0 ? n.z : -n.z, n.y) / a.x * 0.5 + 0.5;
    if (a.y > a.x && a.y > a.z) return float2(n.x, n.y > 0.0 ? n.z : -n.z) / a.y * 0.5 + 0.5;
    return float2(n.z > 0.0 ? n.x : -n.x, n.y) / a.z * 0.5 + 0.5;
}

float2 aabbUV(in float3 p, in float3 n) {
    float3 a = abs(n);
    if (a.x > a.y && a.x > a.z) return float2(n.x < 0.0 ? p.z : -p.z, p.y) * 0.5 + 0.5;
    if (a.y > a.x && a.y > a.z) return float2(p.x, n.y > 0.0 ? p.z : -p.z) * 0.5 + 0.5;
    return float2(n.z > 0.0 ? p.x : -p.x, p.y) * 0.5 + 0.5;
}

// ================ NORMALS ================
float3 sphereNormal(in float3 p) {
    return normalize(p);
}

// ================ RAY INTERSECTION ================
Hit raySphereIntersection(in Ray ray) {
    float3 p = -ray.origin;
    float a = dot(ray.dir, ray.dir);
    float b = dot(ray.dir, p);
    float c = dot(p, p) - 1.0;
    float delta = b * b - a * c;
    if (delta < 0.0) return NO_HIT;

    float sq = sqrt(delta);
    float t = (b - sq) / a;
    if (t < 0.0) t = (b + sq) / a;
    if (t < 0.0) return NO_HIT;

    float3 hitP = ray.origin + ray.dir * t;
    float3 normal = sphereNormal(hitP);
    return makeHit(ray, OBJECT_AABB, t, sphereUV(normal), normal);
}

Hit rayPlaneIntersection(in Ray ray) {
    if (abs(ray.dir.y) <= EPS) return NO_HIT;
    float t = -ray.origin.y / ray.dir.y;
    if (t < EPS) return NO_HIT;

    float3 p = ray.origin + ray.dir * t;
    return makeHit(ray, OBJECT_AABB, t, float2(p.x, p.z), float3(0.0, 1.0, 0.0));
}

Hit rayQuadIntersection(in Ray ray, float tMax) {
    if (ray.dir.z >= -EPS) return NO_HIT; // back-face or parallel
    float t = -ray.origin.z / ray.dir.z;
    if (t < EPS || t >= tMax) return NO_HIT;

    float3 p = ray.origin + ray.dir * t;
    if (abs(p.x) > 0.5 || abs(p.y) > 0.5) return NO_HIT;

    return makeHit(ray, OBJECT_AABB, t, p.xy + float2(0.5), float3(0.0, 0.0, 1.0));
}

Hit rayAabbIntersection(in Ray ray, in float3 aabbMin, in float3 aabbMax, in bool computeNormal) {
    float3 invDir = 1.0 / ray.dir;
    float3 t0 = (aabbMin - ray.origin) * invDir;
    float3 t1 = (aabbMax - ray.origin) * invDir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
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

    float3 normal = float3(0.0);
    if (computeNormal) {
        if (!useFar) {
            if (tmin.x >= tmin.y && tmin.x >= tmin.z) normal = float3(sign(ray.dir.x) < 0.0 ? 1.0 : -1.0, 0.0, 0.0);
            else if (tmin.y >= tmin.z) normal = float3(0.0, sign(ray.dir.y) < 0.0 ? 1.0 : -1.0, 0.0);
            else normal = float3(0.0, 0.0, sign(ray.dir.z) < 0.0 ? 1.0 : -1.0);
        } else {
            if (tmax.x <= tmax.y && tmax.x <= tmax.z) normal = float3(sign(ray.dir.x) < 0.0 ? -1.0 : 1.0, 0.0, 0.0);
            else if (tmax.y <= tmax.z) normal = float3(0.0, sign(ray.dir.y) < 0.0 ? -1.0 : 1.0, 0.0);
            else normal = float3(0.0, 0.0, sign(ray.dir.z) < 0.0 ? -1.0 : 1.0);
        }
    }

    float3 a = abs(normal);
    float3 p = ray.origin + ray.dir * tHit;
    return makeHit(ray, OBJECT_AABB, tHit, aabbUV(p, normal), normal);
}

float rayAabbTNear(in Ray ray, in float3 invDir, in float3 aabbMin, in float3 aabbMax) {
    float3 t0 = (aabbMin - ray.origin) * invDir;
    float3 t1 = (aabbMax - ray.origin) * invDir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);

    if (tFar < max(tNear, EPS)) {
        return INFINITY;
    }

    return tNear;
}

float rayTriangleTNear(in Ray ray, float3 v0, float3 v1, float3 v2, out float u, out float v) {
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 pvec = cross(ray.dir, edge2);
    float det = dot(edge1, pvec);
    if (abs(det) < TRI_EPS) return -1.0;

    float invDet = 1.0 / det;
    float3 tvec = ray.origin - v0;
    u = dot(tvec, pvec) * invDet;
    if (u < -TRI_EPS || u > 1.0 + TRI_EPS) return -1.0;

    float3 qvec = cross(tvec, edge1);
    v = dot(ray.dir, qvec) * invDet;
    if (v < -TRI_EPS || u + v > 1.0 + TRI_EPS) return -1.0;

    float t = dot(edge2, qvec) * invDet;
    return (t >= TRI_EPS) ? t : -1.0;
}

Hit rayMeshIntersection(in Ray ray, in Mesh mesh, bool anyHit, float tMax, inout Statistics stats) {
    if (mesh.bvhNodeCount == 0u) return NO_HIT;

    float3 invDir = 1.0 / ray.dir;
    float3 meshAabbMin = float3(mesh.aabbMinX, mesh.aabbMinY, mesh.aabbMinZ);
    float3 meshAabbMax = float3(mesh.aabbMaxX, mesh.aabbMaxY, mesh.aabbMaxZ);
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
        BvhNode node = scene.bvhNodes[nodeIdx];

        if (node.triangleCount > 0u) {
            stats.triangleChecks += node.triangleCount;
            for (uint i = 0u; i < node.triangleCount; i++) {
                uint base = (node.firstTriangle + i) * 3u;
                uint i0 = scene.indices[base + 0u];
                uint i1 = scene.indices[base + 1u];
                uint i2 = scene.indices[base + 2u];

                float3 v0 = scene.vertices[i0].position;
                float3 v1 = scene.vertices[i1].position;
                float3 v2 = scene.vertices[i2].position;

                float u, v;
                float tLocal = rayTriangleTNear(ray, v0, v1, v2, u, v);
                if (tLocal > 0.0 && tLocal < tClosest) {
                    if (anyHit) return makeHit(ray, OBJECT_AABB, tLocal, float2(0), float3(0.0, 1.0, 0.0));
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
            float t[2] = {
                rayAabbTNear(ray, invDir,
                    float3(node.children[0].minX, node.children[0].minY, node.children[0].minZ),
                    float3(node.children[0].maxX, node.children[0].maxY, node.children[0].maxZ)),
                rayAabbTNear(ray, invDir,
                    float3(node.children[1].minX, node.children[1].minY, node.children[1].minZ),
                    float3(node.children[1].maxX, node.children[1].maxY, node.children[1].maxZ))
            };

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

    float3 normal;
    if (mesh.smoothShading == 1u) {
        float3 n0 = scene.vertices[bestI0].normal;
        float3 n1 = scene.vertices[bestI1].normal;
        float3 n2 = scene.vertices[bestI2].normal;
        normal = normalize((1.0 - bestU - bestV) * n0 + bestU * n1 + bestV * n2);
    } else {
        float3 v0 = scene.vertices[bestI0].position;
        float3 v1 = scene.vertices[bestI1].position;
        float3 v2 = scene.vertices[bestI2].position;
        normal = normalize(cross(v1 - v0, v2 - v0));
    }

    Hit meshHit = makeHit(ray, OBJECT_AABB, tClosest, float2(0), normal);
    if (mesh.hasVertexColor == 1u) {
        float3 c0 = scene.vertices[bestI0].color;
        float3 c1 = scene.vertices[bestI1].color;
        float3 c2 = scene.vertices[bestI2].color;
        meshHit.vertexColor = (1.0 - bestU - bestV) * c0 + bestU * c1 + bestV * c2;
    }
    return meshHit;
}

// ================ SURFACE SAMPLING ================
SurfaceSample sampleSphereSurface(inout RngState rng) {
    SurfaceSample surfaceSample;
    surfaceSample.normal = randomOnSphere(rng);
    surfaceSample.p = surfaceSample.normal;
    return surfaceSample;
}

SurfaceSample sampleBoxSurface(in ModelTransform mt, in float area, inout RngState rng) {
    SurfaceSample surfaceSample;

    float3 axisX = mt.modelMatrix[0].xyz;
    float3 axisY = mt.modelMatrix[1].xyz;
    float3 axisZ = mt.modelMatrix[2].xyz;
    float3 size = 2.0 * float3(length(axisX), length(axisY), length(axisZ));
    float3 pairArea = float3(size.y * size.z, size.z * size.x, size.x * size.y);

    float r = rand(rng) * area;
    float2 uv = float2(rand(rng), rand(rng));

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
        surfaceSample.p = float3(
            x,
            lerp(-1.0, 1.0, uv.x),
            lerp(-1.0, 1.0, uv.y)
        );
        surfaceSample.normal = float3(side, 0.0, 0.0);
    } else if (axis == 1) {
        float y = side < 0.0 ? -1.0 : 1.0;
        surfaceSample.p = float3(
            lerp(-1.0, 1.0, uv.x),
            y,
            lerp(-1.0, 1.0, uv.y)
        );
        surfaceSample.normal = float3(0.0, side, 0.0);
    } else {
        float z = side < 0.0 ? -1.0 : 1.0;
        surfaceSample.p = float3(
            lerp(-1.0, 1.0, uv.x),
            lerp(-1.0, 1.0, uv.y),
            z
        );
        surfaceSample.normal = float3(0.0, 0.0, side);
    }

    return surfaceSample;
}

SurfaceSample sampleQuadSurface(inout RngState rng) {
    SurfaceSample surfaceSample;
    surfaceSample.p = float3(rand(rng) - 0.5, rand(rng) - 0.5, 0.0);
    surfaceSample.normal = float3(0.0, 0.0, 1.0);
    return surfaceSample;
}

SurfaceSample sampleMeshSurface(in Mesh mesh, inout RngState rng) {
    SurfaceSample surfaceSample;
    if (mesh.triangleCount == 0u) {
        surfaceSample.p = float3(0.0);
        surfaceSample.normal = float3(0.0);
        return surfaceSample;
    }

    uint tri = uint(rand(rng) * float(mesh.triangleCount));
    if (tri >= mesh.triangleCount) tri = mesh.triangleCount - 1u;
    uint base = mesh.indexOffset + tri * 3u;
    uint i0 = scene.indices[base + 0u];
    uint i1 = scene.indices[base + 1u];
    uint i2 = scene.indices[base + 2u];

    float3 v0 = scene.vertices[i0].position;
    float3 v1 = scene.vertices[i1].position;
    float3 v2 = scene.vertices[i2].position;

    float r1 = sqrt(rand(rng));
    float r2 = rand(rng);
    surfaceSample.p = v0 * (1.0 - r1) + v1 * (r1 * (1.0 - r2)) + v2 * (r1 * r2);
    surfaceSample.normal = normalize(cross(v1 - v0, v2 - v0));
    return surfaceSample;
}
