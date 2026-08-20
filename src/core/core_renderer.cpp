#include "core_renderer.hpp"

#include "VkSmol/graph/pass/transfer_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include "core/camera/aperture.hpp"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/parameters/parameters.hpp"

RenderResources CoreRenderer::initGraph(RenderGraphBuilder& builder) {
    VkSmol& engine = Core::getEngine();

    lensImageHandle = builder.createImage(
        "LensTexture",
        VK_FORMAT_R8_UNORM,
        aperture::kSize, aperture::kSize, 1,
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    RenderResources resources = PathtraceRenderer::initGraph(builder, engine.getExtent(), "", lensImageHandle);

    TransferPassBuilder exportPass = builder.addTransferPass("ExportPass");
    exportPassHandle = exportPass.getHandle();
    exportPass.setGroup(getGroupHandle());
    exportPass.copyFrom(resources.outputImageHandle);

    exportService.init(
        engine,
        engine.getExtent().width, engine.getExtent().height,
        resources.pixelInfoBufferHandle
    );

    return resources;
}

void CoreRenderer::destroy() {
    VkSmol& engine = Core::getEngine();
    exportService.destroy(engine);
}

void CoreRenderer::buildPipelines() {
    VkSmol& engine = Core::getEngine();
    engine.waitIdle();

    try {
        engine.reloadPipelines();
    } catch (const std::exception& e) {
        Log::error(e.what());
        return;
    }

    Log::success("CoreRenderer", "(Re)Built the pipelines");
}

void CoreRenderer::saveCapture(const std::filesystem::path& path) {
    VkSmol& engine = Core::getEngine();
    exportService.save(engine, engine.getImage(getOutputImageHandle()), path, aovFlags);
}

void CoreRenderer::onAfterDispatch(CommandBuffer& commandBuffer) {
    Core::getEngine().emitBarriers(commandBuffer, exportPassHandle);
}

void CoreRenderer::onResize(uint32_t width, uint32_t height) {
    VkSmol& engine = Core::getEngine();
    if (engine.isHeadless())
        engine.getExtent() = { width, height };
    exportService.resize(engine, width, height);
}

void CoreRenderer::bindParameters() {
    ParameterRegistry& parameters = Core::getParameters();
    parameters.bind<bool>("renderer/denoising", [this](bool v) {
        compositingUBO.denoisingEnabled = static_cast<int>(v);
    });

    parameters.bind("renderer/sampling/max_bounces", &pathtracerUBO.render.maxBounces);
    parameters.bind("renderer/sampling/adaptive_warmup", &pathtracerUBO.render.varianceWarmupSamples);

    parameters.bind<bool>("renderer/sampling/importance_sampling", [this](bool v) {
        pathtracerUBO.render.importanceSampling = static_cast<int>(v);
    });

    parameters.bind<bool>("renderer/sampling/clamp", [this](bool v) {
        pathtracerUBO.render.clipAccumulation = static_cast<int>(v);
    });
    parameters.bind("renderer/sampling/clamp_threshold", &pathtracerUBO.render.clipThreshold);

    parameters.bind<bool>("renderer/sampling/adaptive_sampling", [this](bool v) {
        pathtracerUBO.render.varianceSampling = static_cast<int>(v);
    });

    parameters.bind<int>("renderer/viewport/max_samples", [this](int n) {
        if (Core::getRenderMode() == RenderMode::Preview)
            setTargetSampleCount(n > 0 ? n : SampleAccumulator::kUnboundedSamples);
    });

    parameters.bind<glm::ivec2>("renderer/output/render_size", [](glm::ivec2 size) {
        Core::requestResize(size.x, size.y);
    });

    parameters.bind("scene/light_mode", &pathtracerUBO.render.lightMode);

    for (const AOVChannel& channel : kAOVChannels)
        parameters.bind(std::string("renderer/aov/") + channel.name, &(aovFlags.*channel.flag));
}
