#include "core_renderer.hpp"

#include <string_view>

#include <glm/glm.hpp>

#include "core/camera/aperture.hpp"
#include "core/core.hpp"
#include "core/fields/parameters.hpp"
#include "utils/resources.hpp"

void CoreRenderer::init() {
    VkSmol::Engine& engine = Core::getEngine();

    lensImageHandle = engine.createImage("LensTexture", VK_FORMAT_R8_UNORM, aperture::kSize, aperture::kSize);

    const std::string_view blueNoise = Resources::find("builtin:/noise/blue_noise.bin").value();
    blueNoiseBufferHandle = engine.createBuffer("BlueNoiseBuffer", blueNoise.size(), blueNoise.data());

    const glm::ivec2 renderSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
    const VkExtent2D extent = {static_cast<uint32_t>(renderSize.x), static_cast<uint32_t>(renderSize.y)};
    PathtraceRenderer::init(extent, "", lensImageHandle, blueNoiseBufferHandle);

    exportService.init(extent.width, extent.height, getResources().pixelInfoBufferHandle);
}

void CoreRenderer::saveCapture(const std::filesystem::path& path) {
    exportService.save(Core::getEngine(), getOutputImageHandle(), path, aovFlags);
}

void CoreRenderer::onResize(uint32_t width, uint32_t height) { exportService.resize(width, height); }

void CoreRenderer::bindParameters() {
    ParameterRegistry& parameters = Core::getParameters();

    parameters.bind("renderer/sampling/max_bounces", &pathtracerUBO.render.maxBounces);
    parameters.bind("renderer/sampling/adaptive_warmup", &pathtracerUBO.render.varianceWarmupSamples);

    parameters.bind<bool>("renderer/sampling/importance_sampling",
                          [this](bool v) { pathtracerUBO.render.importanceSampling = static_cast<int>(v); });

    parameters.bind<bool>("renderer/sampling/clamp",
                          [this](bool v) { pathtracerUBO.render.clipAccumulation = static_cast<int>(v); });
    parameters.bind("renderer/sampling/clamp_threshold", &pathtracerUBO.render.clipThreshold);

    parameters.bind<bool>("renderer/sampling/adaptive_sampling",
                          [this](bool v) { pathtracerUBO.render.varianceSampling = static_cast<int>(v); });

    parameters.bind<int>("renderer/viewport/max_samples", [this](int n) {
        if (Core::getRenderMode() == RenderMode::Preview)
            setTargetSampleCount(n > 0 ? n : SampleAccumulator::kUnboundedSamples);
    });

    parameters.bind<glm::ivec2>("renderer/output/render_size",
                                [](glm::ivec2 size) { Core::requestResize(size.x, size.y); });

    for (const AOVChannel& channel : kAOVChannels)
        parameters.bind(std::string("renderer/aov/") + channel.name, &(aovFlags.*channel.flag));
}
