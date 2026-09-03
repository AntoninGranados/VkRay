#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials/material_utils.glsl"
#include "objects.glsl"

Hit rayObjectIntersection(in Ray ray, in Object obj, bool anyHit, float tMax, inout Statistics stats) {
    switch (obj.type) {
        case obj_Sphere: return raySphereIntersection(ray, obj, sphereBuffer.spheres[obj.id], anyHit, tMax, stats);
        case obj_Plane:  return rayPlaneIntersection(ray, obj, planeBuffer.planes[obj.id], anyHit, tMax, stats);
        case obj_Box:    return rayBoxIntersection(ray, obj, boxBuffer.boxes[obj.id], anyHit, tMax, stats);
        case obj_Quad:   return rayQuadIntersection(ray, obj, quadBuffer.quads[obj.id], anyHit, tMax, stats);
        case obj_Mesh:   return rayMeshIntersection(ray, obj, meshBuffer.meshes[obj.id], anyHit, tMax, stats);
        default:         return NO_HIT;
    }
}

Material getMaterial(in Object obj) {
    return materialBuffer.materials[obj.materialSlot];
}

SurfaceSample sampleSurface(in Object obj, in float area, inout RngState rng) {
    switch (obj.type) {
        case obj_Sphere: return sampleSphereSurface(sphereBuffer.spheres[obj.id], area, rng);
        case obj_Plane:  return SurfaceSample(vec3(0.0), vec3(0.0));
        case obj_Box:    return sampleBoxSurface(boxBuffer.boxes[obj.id], area, rng);
        case obj_Quad:   return sampleQuadSurface(quadBuffer.quads[obj.id], area, rng);
        case obj_Mesh:   return sampleMeshSurface(meshBuffer.meshes[obj.id], area, rng);
        default:         return SurfaceSample(vec3(0.0), vec3(0.0));
    }
}

#endif
