struct OverlayLine {
    float3 start;
    float3 end;
    float3 color;
    uint   onTop;
};

struct OverlayConstants {
    float4x4 viewProjection;
    float2   viewportSize;
    float    lineWidth;
};

struct LineVertex {
    float4 position : SV_Position;
    float3 color : COLOR0;
    noperspective float edgeDistance : TEXCOORD0;
    nointerpolation uint onTop : TEXCOORD1;
};

[vk::push_constant] ConstantBuffer<OverlayConstants> constants;
StructuredBuffer<OverlayLine> lines;
Sampler2D<float> depthTex;

static const float antialiasingWidth = 1.0;
static const float2 quadCorners[6] = {
    float2(0.0, -1.0), float2(0.0, 1.0), float2(1.0, -1.0), float2(1.0, -1.0), float2(0.0, 1.0), float2(1.0, 1.0),
};

[shader("vertex")]
LineVertex vertexMain(uint vertexId : SV_VertexID) {
    OverlayLine line = lines[vertexId / 6];
    float2 corner = quadCorners[vertexId % 6];

    LineVertex output;
    output.color = line.color;
    output.onTop = line.onTop;
    output.edgeDistance = 0.0;

    float4 start = mul(float4(line.start, 1.0), constants.viewProjection);
    float4 end   = mul(float4(line.end, 1.0), constants.viewProjection);
    float startInside = start.w - start.z;
    float endInside   = end.w - end.z;
    if (startInside < 0.0 && endInside < 0.0) {
        output.position = float4(0.0, 0.0, -1.0, 1.0);
        return output;
    }
    if (startInside < 0.0) start = lerp(start, end, startInside / (startInside - endInside));
    if (endInside < 0.0) end = lerp(end, start, endInside / (endInside - startInside));

    float2 halfViewport = constants.viewportSize * 0.5;
    float2 delta = (end.xy / end.w - start.xy / start.w) * halfViewport;
    float  lengthPixels = length(delta);
    float2 direction = lengthPixels > 1e-6 ? delta / lengthPixels : float2(1.0, 0.0);
    float2 normal = float2(-direction.y, direction.x);

    float extent = constants.lineWidth * 0.5 + antialiasingWidth;
    float4 position = corner.x > 0.5 ? end : start;
    position.xy += normal * (corner.y * extent) / halfViewport * position.w;

    output.position = position;
    output.edgeDistance = corner.y * extent;
    return output;
}

[shader("fragment")]
float4 fragmentMain(LineVertex input) : SV_Target {
    if (input.onTop == 0u && input.position.z < depthTex.Load(int3(int2(input.position.xy), 0))) discard;
    float coverage = saturate(constants.lineWidth * 0.5 + 0.5 - abs(input.edgeDistance));
    return float4(input.color, coverage);
}
