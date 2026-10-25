import "../core/scene.slang";
import "../core/materials/materials.slang";
import "../core/objects.slang";
import "../core/motion.slang";
import "../core/random/utils.slang";

struct UnitVertex {
    float3 position;
    float3 normal;
};

struct RasterConstants {
    float4x4 viewProjection;
    float3   cameraPosition;
    uint     objectIndex;
    uint     firstIndex;
};

struct RasterVertex {
    float4 position : SV_Position;
    float3 worldPosition : POSITION0;
    float3 worldNormal : NORMAL0;
    float3 localPosition : POSITION1;
    float3 localNormal : NORMAL1;
    float3 vertexColor : COLOR0;
};

[vk::push_constant] ConstantBuffer<RasterConstants> constants;
StructuredBuffer<UnitVertex> unitVertices;
StructuredBuffer<uint> unitIndices;

static const float ambientLight = 0.2;

float2 surfaceUV(Object object, float3 localPosition, float3 localNormal) {
    switch (object.type) {
        case obj_Sphere: return sphereUV(normalize(localPosition));
        case obj_Box:    return aabbUV(localPosition, localNormal);
        case obj_Quad:   return localPosition.xy + float2(0.5);
        case obj_Plane:  return localPosition.xz;
        default:         return float2(0.0);
    }
}

[shader("vertex")]
RasterVertex vertexMain(uint vertexId : SV_VertexID) {
    Object object = loadObject(constants.objectIndex);

    RasterVertex output;
    output.vertexColor = float3(1.0);
    if (object.type == obj_Mesh) {
        Vertex meshVertex = scene.vertices[scene.indices[constants.firstIndex + vertexId]];
        output.localPosition = meshVertex.position;
        output.localNormal = meshVertex.normal;
        if (scene.meshes[object.id].hasVertexColor == 1u) output.vertexColor = meshVertex.color;
    } else {
        UnitVertex unitVertex = unitVertices[unitIndices[constants.firstIndex + vertexId]];
        output.localPosition = unitVertex.position;
        output.localNormal = unitVertex.normal;
    }

    ModelTransform transform = sampleMotion(object.motionOffset);
    output.worldPosition = mul(float4(output.localPosition, 1.0), transform.modelMatrix).xyz;
    output.worldNormal = mul(output.localNormal, float3x3(transpose(transform.invModelMatrix)));
    output.position = mul(float4(output.worldPosition, 1.0), constants.viewProjection);
    return output;
}

[shader("fragment")]
float4 fragmentMain(RasterVertex input) : SV_Target {
    Object object = loadObject(constants.objectIndex);
    float3 toCamera = normalize(constants.cameraPosition - input.worldPosition);

    float3 normal = normalize(input.worldNormal);
    if (object.type == obj_Mesh && scene.meshes[object.id].smoothShading == 0u) {
        float3 faceNormal = normalize(cross(ddx(input.worldPosition), ddy(input.worldPosition)));
        normal = dot(faceNormal, normal) < 0.0 ? -faceNormal : faceNormal;
    }
    bool frontFace = dot(normal, toCamera) >= 0.0;

    SurfacePoint surface = SurfacePoint(
        object, input.worldPosition, frontFace ? normal : -normal,
        surfaceUV(object, input.localPosition, input.localNormal), frontFace, input.vertexColor
    );
    RngState rng = initRngState(uint2(input.position.xy), 0u);
    IMaterial material = makeMaterial(scene.materials[object.materialSlot]);
    BsdfProperties properties = material.evaluate(surface, toCamera, rng).properties();

    float headlight = max(dot(surface.normal, toCamera), 0.0);
    return float4(properties.albedo * (ambientLight + (1.0 - ambientLight) * headlight), 1.0);
}
