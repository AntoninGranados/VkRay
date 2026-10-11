__exported import "utils.slang";
__exported import "materials/material_utils.slang";

struct Scene {
    StructuredBuffer<Vertex> vertices;
    StructuredBuffer<uint> indices;
    StructuredBuffer<BvhNode> bvhNodes;
    StructuredBuffer<Mesh> meshes;
    StructuredBuffer<Material> materials;
    StructuredBuffer<float> pluginParams;
    ByteAddressBuffer objects;
    StructuredBuffer<MotionSample> motion;
};

ParameterBlock<Scene> scene;

uint objectCount() {
    return scene.objects.Load<uint>(0);
}

Object loadObject(uint index) {
    return scene.objects.Load<Object>(sizeof(uint) + index * sizeof(Object));
}

ResolvedMaterial unpackMaterial(in Material mat) {
    ResolvedMaterial resolved;
    resolved.type = mat.type;
    for (int i = 0; i < MATERIAL_PAYLOAD_SIZE; i++) resolved.payload[i] = scene.pluginParams[mat.base + i];
    return resolved;
}

ResolvedMaterial defaultMaterial() {
    return unpackMaterial(scene.materials[0]);
}
