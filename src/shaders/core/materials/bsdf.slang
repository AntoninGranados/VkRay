import "../scene.slang";
import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";

struct BsdfProperties {
    int type;
    float3 albedo;
    float roughness;
    float3 emission = float3(0.0);
    bool passThrough = false;
};

[anyValueSize(80)]
interface IBsdf {
    [mutating] void shade(inout SurfacePoint surface) {}
    BSDFEval eval(SurfacePoint surface, float3 wo, float3 wi);
    BSDFSample sample(SurfacePoint surface, float3 wo, inout RngState rng);
    BsdfProperties properties();
    Optional<MediumEntry> medium(Object object) { return none; }
    bool isMediumBoundary() { return false; }
    property float3 albedo { get; set; }
}

[anyValueSize(4)]
interface IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng);
    bool isMediumBoundary() { return false; }
}

extension<T : IMaterial> Script<T> : IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng) {
        RngState scriptRng = rng;
        return loadParams<T>(base).evaluate(surface, wo, scriptRng);
    }
}

extension<T : IBsdf> Script<T> : IMaterial {
    IBsdf evaluate(inout SurfacePoint surface, float3 wo, inout RngState rng) {
        T bsdf = loadParams<T>(base);
        bsdf.albedo *= surface.vertexColor;
        bsdf.shade(surface);
        surface.normal = ensureValidReflection(surface.geometricNormal, wo, surface.normal);
        return bsdf;
    }

    override bool isMediumBoundary() {
        return loadParams<T>(base).isMediumBoundary();
    }
}
