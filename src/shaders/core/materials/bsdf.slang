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

[anyValueSize(44)]
interface IBsdf {
    BSDFEval eval(Hit hit, float3 wo, float3 wi);
    BSDFSample sample(Hit hit, float3 wo, inout RngState rng);
    BsdfProperties properties();
    Optional<MediumEntry> medium(Object object) { return none; }
}

[anyValueSize(4)]
interface IMaterial {
    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng);
    bool isMediumBoundary() { return false; }
}

struct ScriptMaterial<T : IMaterial> : IMaterial {
    uint base;

    IBsdf evaluate(inout Hit hit, float3 wo, inout RngState rng) {
        RngState scriptRng = rng;
        return loadParams<T>(base).evaluate(hit, wo, scriptRng);
    }
}
