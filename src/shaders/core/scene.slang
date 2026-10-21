__exported import "utils.slang";
__exported import "materials/material_utils.slang";

struct Scene {
    StructuredBuffer<Vertex> vertices;
    StructuredBuffer<uint> indices;
    StructuredBuffer<BvhNode> bvhNodes;
    StructuredBuffer<Mesh> meshes;
    StructuredBuffer<Material> materials;
    ByteAddressBuffer pluginParams;
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

T loadParams<T>(uint index) {
    return scene.pluginParams.Load<T>(index * sizeof(float));
}

float loadParam(uint index) {
    return loadParams<float>(index);
}
