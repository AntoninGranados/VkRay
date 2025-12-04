#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"

float rayObjectIntersection(in Ray ray, in Object obj) {
    if (obj.type == obj_Sphere)
        return raySphereIntersection(ray, sphereBuffer.spheres[obj.id]);
    else if (obj.type == obj_Plane)
        return rayPlaneIntersection(ray, planeBuffer.planes[obj.id]);
    else if (obj.type == obj_Box)
        return rayBoxIntersection(ray, boxBuffer.boxes[obj.id]);
    return -1;
}

Material getMaterial(in Object obj) {
    if (obj.type == obj_Sphere)
        return sphereBuffer.spheres[obj.id].mat;
    else if (obj.type == obj_Plane)
        return planeBuffer.planes[obj.id].mat;
    else if (obj.type == obj_Box)
        return boxBuffer.boxes[obj.id].mat;
    return DEFAULT_MATERIAL;
}

vec3 getNormal(in Object obj, in vec3 p) {
    if (obj.type == obj_Sphere)
        return sphereNormal(sphereBuffer.spheres[obj.id], p);
    else if (obj.type == obj_Plane)
        return planeNormal(planeBuffer.planes[obj.id], p);
    else if (obj.type == obj_Box)
        return boxNormal(boxBuffer.boxes[obj.id], p);
    return vec3(0);
}

#endif
