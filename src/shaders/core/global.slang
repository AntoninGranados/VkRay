import "scene.slang";
import "random/utils.slang";
import "motion.slang";
__exported import "objects.slang";

Hit rayObjectIntersection(in Ray ray, in Object obj, bool anyHit, float tMax, inout Statistics stats) {
    ModelTransform mt = sampleMotion(obj.motionOffset);
    Ray localRay = Ray(
        mul(float4(ray.origin, 1.0), mt.invModelMatrix).xyz,
        mul(float4(ray.dir, 0.0), mt.invModelMatrix).xyz
    );

    Hit localHit;
    switch (obj.type) {
        case obj_Sphere: localHit = raySphereIntersection(localRay); break;
        case obj_Plane:  localHit = rayPlaneIntersection(localRay); break;
        case obj_Box:    localHit = rayAabbIntersection(localRay, float3(-1.0), float3(1.0), true); break;
        case obj_Quad:   localHit = rayQuadIntersection(localRay, tMax); break;
        case obj_Mesh:   localHit = rayMeshIntersection(localRay, scene.meshes[obj.id], anyHit, tMax, stats); break;
        default:         return NO_HIT;
    }
    if (!foundIntersection(localHit)) return localHit;

    float3x3 normalMat = float3x3(transpose(mt.invModelMatrix));
    float3 localOutwardNormal = localHit.frontFace ? localHit.normal : -localHit.normal;
    float3 worldNormal = normalize(mul(localOutwardNormal, normalMat));
    float3 worldP = mul(float4(localHit.p, 1.0), mt.modelMatrix).xyz;
    float tWorld = dot(worldP - ray.origin, ray.dir);

    Hit hit = makeHit(ray, obj, tWorld, localHit.uv, worldNormal);
    hit.vertexColor = localHit.vertexColor;
    return hit;
}

Hit intersection(in Ray ray, bool anyHit, float tMax, inout Statistics stats) {
    Hit bestHit = NO_HIT;
    for (int i = 0; i < objectCount(); i++) {
        Hit hit = rayObjectIntersection(ray, loadObject(i), anyHit, tMax, stats);
        if (foundIntersection(hit) && hit.t < tMax) {
            if (anyHit) return hit;
            if (hit.t < bestHit.t) bestHit = hit;
        }
    }
    return bestHit;
}

Material getMaterial(in Object obj) {
    return scene.materials[obj.materialSlot];
}

SurfaceSample sampleSurface(in Object obj, in float area, inout RngState rng) {
    ModelTransform mt = sampleMotion(obj.motionOffset);

    SurfaceSample localSample;
    switch (obj.type) {
        case obj_Sphere: localSample = sampleSphereSurface(rng); break;
        case obj_Plane:  return SurfaceSample(float3(0.0), float3(0.0));
        case obj_Box:    localSample = sampleBoxSurface(mt, area, rng); break;
        case obj_Quad:   localSample = sampleQuadSurface(rng); break;
        case obj_Mesh:   localSample = sampleMeshSurface(scene.meshes[obj.id], rng); break;
        default:         return SurfaceSample(float3(0.0), float3(0.0));
    }

    float3x3 normalMat = float3x3(transpose(mt.invModelMatrix));
    SurfaceSample worldSample;
    worldSample.p = mul(float4(localSample.p, 1.0), mt.modelMatrix).xyz;
    worldSample.normal = normalize(mul(localSample.normal, normalMat));
    return worldSample;
}
