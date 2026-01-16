#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"

Hit rayObjectIntersection(in Ray ray, in Object obj) {
    switch (obj.type) {
        case obj_Sphere: return raySphereIntersection(ray, obj, sphereBuffer.spheres[obj.id]);
        case obj_Plane:  return rayPlaneIntersection(ray, obj, planeBuffer.planes[obj.id]);
        case obj_Box:    return rayBoxIntersection(ray, obj, boxBuffer.boxes[obj.id]);
        case obj_Mesh:   return rayMeshIntersection(ray, obj, meshBuffer.meshes[obj.id]);
        default:         return Hit(vec3(0), vec3(0), INFINITY, true, OBJECT_NONE);
    }
}

Material getMaterial(in Object obj) {
    switch (obj.type) {
        case obj_Sphere: return materialBuffer.materials[sphereBuffer.spheres[obj.id].materialHandle];
        case obj_Plane:  return materialBuffer.materials[planeBuffer.planes[obj.id].materialHandle];
        case obj_Box:    return materialBuffer.materials[boxBuffer.boxes[obj.id].materialHandle];
        case obj_Mesh:   return materialBuffer.materials[meshBuffer.meshes[obj.id].materialHandle];
        default:         return DEFAULT_MATERIAL;
    }
}

SurfaceSample sampleSurface(in Object obj, in float area, inout uint seed) {
    switch (obj.type) {
        case obj_Sphere: return sampleSphereSurface(sphereBuffer.spheres[obj.id], area, seed);
        case obj_Plane:  return SurfaceSample(vec3(0.0), vec3(0.0));
        case obj_Box:    return sampleBoxSurface(boxBuffer.boxes[obj.id], area, seed);
        case obj_Mesh:   return sampleMeshSurface(meshBuffer.meshes[obj.id], area, seed);
        default:         return SurfaceSample(vec3(0.0), vec3(0.0));
    }
}

#endif
