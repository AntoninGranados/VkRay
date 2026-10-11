import "../utils.slang";
import "../random/utils.slang";

import "material_utils.slang";

BSDFEval evalDielectricBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, in float3 wi) {
    return BSDFEval(
        float3(0.0),
        -1.0
    );
}

BSDFSample sampleDielectricBSDF(in ResolvedMaterial mat, in Hit hit, in float3 wo, inout RngState rng) {
    float etaI, etaT;
    interfaceIors(hit, mat.dielectricIor, etaI, etaT);

    float3 normal = hit.normal + randomOnSphere(rng) * mat.dielectricRoughness;
    if (length(normal) < EPS) normal = hit.normal;
    else normal = normalize(normal);

    float eta = etaI / etaT;

    float cosTheta = clamp(dot(wo, normal), 0.0, 1.0);
    float F = fresnelDielectric(cosTheta, etaI, etaT);

    float3 wi;
    float3 weight;
    float pdf;
    if (rand(rng) < F) {
        wi = reflect(-wo, normal);

        pdf = max(F, EPS);
        weight = float3(1.0);
    } else {
        wi = refract(-wo, normal, eta);
        float etaFactor = (etaI * etaI) / (etaT * etaT);

        pdf = max(1.0 - F, EPS);
        weight = float3(etaFactor);

        crossInterface(mat, hit);
    }

    bool entering = (dot(wi, hit.normal) < 0.0) == hit.frontFace;

    BSDFSample bsdf;
    bsdf.wi                   = wi;
    bsdf.weight               = weight;
    bsdf.pdf                  = pdf;
    bsdf.isDelta              = true;
    bsdf.medium.isDielectric  = entering;
    bsdf.medium.isVolume      = entering && mat.dielectricDensity > 0.0;
    bsdf.medium.absorption    = mat.albedo;
    bsdf.medium.density       = mat.dielectricDensity;
    bsdf.medium.scatterAlbedo = mat.dielectricTransmission;
    bsdf.medium.anisotropic   = mat.dielectricAnisotropic;
    return bsdf;
}
