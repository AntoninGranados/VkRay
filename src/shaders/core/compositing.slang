import "compositing_api.slang";

struct PassUBO {
    int slot;
    int paramsBase;
    int passId;
};

ConstantBuffer<PassUBO> passUbo;
[[vk::image_format("rgba32f")]] RWTexture2D<float4> destinationImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID) {
    int2 texSize = textureSize(sourceTexture);
    int2 pixelCoord = int2(dispatchThreadId.xy);
    if (pixelCoord.x >= texSize.x || pixelCoord.y >= texSize.y) return;

    ICompositingPass pass = createDynamicObject<ICompositingPass, uint>(uint(passUbo.slot), uint(passUbo.paramsBase));
    destinationImage[pixelCoord] = pass.evaluate(pixelCoord, texSize, passUbo.passId);
}
