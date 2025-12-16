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
        return materialBuffer.materials[sphereBuffer.spheres[obj.id].materialHandle];
    else if (obj.type == obj_Plane)
        return materialBuffer.materials[planeBuffer.planes[obj.id].materialHandle];
    else if (obj.type == obj_Box)
        return materialBuffer.materials[boxBuffer.boxes[obj.id].materialHandle];
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

int getLightId() {
    for (int i = 0; i < objectBuffer.objectCount; i++) {
        if (getMaterial(objectBuffer.objects[i]).type == mat_Emissive) {
            return i;
        }
    }
    return -1;
}

#endif
