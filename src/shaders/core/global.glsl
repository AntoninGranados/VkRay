#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials/material_utils.glsl"
#include "objects.glsl"

Hit rayObjectIntersection(in Ray ray, in Object obj, bool anyHit, float tMax, inout Statistics stats) {
    ModelTransform mt = sampleMotion(obj.motionOffset);
    Ray localRay = Ray(
        (mt.invModelMatrix * vec4(ray.origin, 1.0)).xyz,
        (mt.invModelMatrix * vec4(ray.dir, 0.0)).xyz
    );

    Hit localHit;
    switch (obj.type) {
        case obj_Sphere: localHit = raySphereIntersection(localRay); break;
        case obj_Plane:  localHit = rayPlaneIntersection(localRay); break;
        case obj_Box:    localHit = rayAabbIntersection(localRay, vec3(-1.0), vec3(1.0), true); break;
        case obj_Quad:   localHit = rayQuadIntersection(localRay, tMax); break;
        case obj_Mesh:   localHit = rayMeshIntersection(localRay, meshBuffer.meshes[obj.id], anyHit, tMax, stats); break;
        default:         return NO_HIT;
    }
    if (!foundIntersection(localHit)) return localHit;

    mat3 normalMat = mat3(transpose(mt.invModelMatrix));
    vec3 localOutwardNormal = localHit.frontFace ? localHit.normal : -localHit.normal;
    vec3 worldNormal = normalize(normalMat * localOutwardNormal);
    vec3 worldP = (mt.modelMatrix * vec4(localHit.p, 1.0)).xyz;
    float tWorld = dot(worldP - ray.origin, ray.dir);

    Hit hit = makeHit(ray, obj, tWorld, localHit.uv, worldNormal);
    hit.vertexColor = localHit.vertexColor;
    return hit;
}

Material getMaterial(in Object obj) {
    return materialBuffer.materials[obj.materialSlot];
}

SurfaceSample sampleSurface(in Object obj, in float area, inout RngState rng) {
    ModelTransform mt = sampleMotion(obj.motionOffset);

    SurfaceSample localSample;
    switch (obj.type) {
        case obj_Sphere: localSample = sampleSphereSurface(rng); break;
        case obj_Plane:  return SurfaceSample(vec3(0.0), vec3(0.0));
        case obj_Box:    localSample = sampleBoxSurface(mt, area, rng); break;
        case obj_Quad:   localSample = sampleQuadSurface(rng); break;
        case obj_Mesh:   localSample = sampleMeshSurface(meshBuffer.meshes[obj.id], rng); break;
        default:         return SurfaceSample(vec3(0.0), vec3(0.0));
    }

    mat3 normalMat = mat3(transpose(mt.invModelMatrix));
    SurfaceSample worldSample;
    worldSample.p = (mt.modelMatrix * vec4(localSample.p, 1.0)).xyz;
    worldSample.normal = normalize(normalMat * localSample.normal);
    return worldSample;
}

#endif
