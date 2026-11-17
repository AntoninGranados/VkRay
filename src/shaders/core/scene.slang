__exported import "utils.slang";
__exported import "materials/material_utils.slang";

static const uint kMaxTextures = 256;

struct Scene {
    StructuredBuffer<Vertex> vertices;
    StructuredBuffer<uint> indices;
    StructuredBuffer<BvhNode> bvhNodes;
    StructuredBuffer<Mesh> meshes;
    StructuredBuffer<Material> materials;
    ByteAddressBuffer pluginParams;
    ByteAddressBuffer objects;
    StructuredBuffer<MotionSample> motion;
    Sampler2D textures[kMaxTextures];
};

ParameterBlock<Scene> scene;

float4 sampleTexture(int index, float2 uv) {
    uint slot = min(uint(index), kMaxTextures - 1u);
    return scene.textures[NonUniformResourceIndex(slot)].SampleLevel(frac(uv), 0.0);
}

uint objectCount() {
    return scene.objects.Load<uint>(0);
}

Object loadObject(uint index) {
    return scene.objects.Load<Object>(sizeof(uint) + index * sizeof(Object));
}

T loadParams<T>(uint index) {
    return scene.pluginParams.Load<T>(index * sizeof(float));
}

float loadParam(uint index) {
    return loadParams<float>(index);
}
