#ifndef GLOBAL_GLSL
#define GLOBAL_GLSL

#include "inputs.glsl"
#include "utils.glsl"
#include "materials.glsl"
#include "objects.glsl"

Material getMaterial(in Object obj) {
    if (obj.type == obj_Sphere)
        return ssbo.spheres[obj.id].mat;
    else if (obj.type == obj_Plane)
        return planes[obj.id].mat;
    else if (obj.type == obj_Box)
        return boxes[obj.id].mat;
    return DEFAULT_MATERIAL;
}

vec3 getNormal(in Object obj, in vec3 p) {
    if (obj.type == obj_Sphere)
        return sphereNormal(ssbo.spheres[obj.id], p);
    else if (obj.type == obj_Plane)
        return planeNormal(planes[obj.id], p);
    else if (obj.type == obj_Box)
        return boxNormal(boxes[obj.id], p);
    return vec3(0);
}

#endif
