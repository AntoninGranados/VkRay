__exported import "../dsl_math_utils.slang";

Sampler2D sourceTexture;
Sampler2D originalTexture;
RWStructuredBuffer<PixelInfo> pixelInfoBuffer;
StructuredBuffer<float> pluginParams;

PixelInfo fetchPixelInfoOriginal(int2 coord, int2 texSize) {
    int2 c = clamp(coord, int2(0), texSize - int2(1));
    uint index = uint(c.y * texSize.x + c.x);
    return pixelInfoBuffer[index];
}

float4 sampleSource(int2 coord, int2 texSize) {
    int2 c = clamp(coord, int2(0), texSize - int2(1));
    return sourceTexture.Load(int3(c, 0));
}

float4 sampleOriginal(int2 coord, int2 texSize) {
    int2 c = clamp(coord, int2(0), texSize - int2(1));
    return originalTexture.Load(int3(c, 0));
}
