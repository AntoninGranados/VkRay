import "../scene.slang";
import "../random/utils.slang";

__exported import "bsdf.slang";
__exported import "principled.slang";
__exported import "diffuse.slang";
__exported import "emissive.slang";
__exported import "metal.slang";
__exported import "glossy.slang";
__exported import "dielectric.slang";
__exported import "volume.slang";

DiffuseBsdf Diffuse(float3 albedo) {
    return DiffuseBsdf(albedo);
}

EmissiveBsdf Emissive(float3 albedo, float strength) {
    return EmissiveBsdf(albedo, strength);
}

GlossyBsdf Glossy(float3 albedo, float roughness, float ior) {
    return GlossyBsdf(albedo, roughness, ior);
}

MetalBsdf Metal(float3 albedo, float roughness) {
    return MetalBsdf(albedo, roughness);
}

DielectricBsdf Dielectric(float3 albedo, float roughness, float ior, float transmission, float density, float anisotropic) {
    return DielectricBsdf(albedo, roughness, ior, density, transmission, anisotropic, 0);
}

// TODO: implement all parameters
PrincipledBsdf Principled(float3 albedo, float roughness, float metalness, float ior, float alpha) {
    PrincipledBsdf bsdf = PrincipledBsdf();
    bsdf.albedo = albedo;
    bsdf.roughness = roughness;
    bsdf.metalness = metalness;
    bsdf.ior = ior;
    bsdf.alpha = alpha;
    return bsdf;
}

struct DefaultMaterial : IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng) {
        return DiffuseBsdf(float3(0.5, 0.0, 0.5));
    }
}

IMaterial makeMaterial(in Material mat) {
    return createDynamicObject<IMaterial, uint>(uint(mat.type), mat.base);
}
