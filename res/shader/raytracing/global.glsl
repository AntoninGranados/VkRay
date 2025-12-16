#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"

float rayObjectIntersection(in Ray ray, in Object obj) {
    switch (obj.type) {
        case obj_Sphere: return raySphereIntersection(ray, sphereBuffer.spheres[obj.id]);
        case obj_Plane:  return rayPlaneIntersection(ray, planeBuffer.planes[obj.id]);
        case obj_Box:    return rayBoxIntersection(ray, boxBuffer.boxes[obj.id]);
        default:         return -1;
    }
}

Material getMaterial(in Object obj) {
    switch (obj.type) {
        case obj_Sphere: return materialBuffer.materials[sphereBuffer.spheres[obj.id].materialHandle];
        case obj_Plane:  return materialBuffer.materials[planeBuffer.planes[obj.id].materialHandle];
        case obj_Box:    return materialBuffer.materials[boxBuffer.boxes[obj.id].materialHandle];
        default:         return DEFAULT_MATERIAL;
    }
}

vec3 getNormal(in Object obj, in vec3 p) {
    switch (obj.type) {
        case obj_Sphere: return sphereNormal(sphereBuffer.spheres[obj.id], p);
        case obj_Plane:  return planeNormal(planeBuffer.planes[obj.id], p);
        case obj_Box:    return boxNormal(boxBuffer.boxes[obj.id], p);
        default:         return vec3(0);
    }
}

SurfaceSample sampleSurface(in Object obj, inout vec3 seed) {
    switch (obj.type) {
        case obj_Sphere: return sampleSphereSurface(sphereBuffer.spheres[obj.id], seed);
        case obj_Plane:  return SurfaceSample(vec3(0.0), vec3(0.0), -1.0);
        case obj_Box:    return sampleBoxSurface(boxBuffer.boxes[obj.id], seed);
        default:         return SurfaceSample(vec3(0.0), vec3(0.0), -1.0);
    }
}

#endif
