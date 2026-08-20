#include "scene_renderer.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/transfer_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/image/image.hpp"

#include "core/camera/aperture.hpp"

#include "utils/log.hpp"
#include "core/core.hpp"
#include "core/parameters/parameters.hpp"
#include "core/structures.hpp"

RenderResources SceneRenderer::initGraph(RenderGraphBuilder& builder) {
    VkSmol& engine = Core::getEngine();
    coreGroupHandle = builder.addSubmissionGroup("Core");

    previousPathtracingImageHandle = builder.createImage(
        "PreviousPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height, 1,
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        VK_IMAGE_USAGE_STORAGE_BIT
    );
    currentPathtracingImageHandle = builder.createImage(
        "CurrentPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    resources.outputImageHandle = builder.createImage(
        "OutputImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        engine.getExtent().width, engine.getExtent().height
    );
    lensImageHandle = builder.createImage(
        "LensTexture",
        VK_FORMAT_R8_UNORM,
        aperture::kSize, aperture::kSize, 1,
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    VkExtent2D ext        = engine.getExtent();
    pathtracingUBOHandle  = builder.createPerFrameBuffer("PathtracingUBO", sizeof(PathtracerUBO),  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    compositingUBOHandle  = builder.createPerFrameBuffer("CompositingUBO", sizeof(CompositingUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    resources.pixelInfoBufferHandle = builder.createBuffer(
        "PixelInfoBuffer",
        static_cast<size_t>(ext.width) * ext.height * sizeof(PixelInfo),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    // Scene buffers must be created before passes so binding slots can be declared
    resources.sceneHandles.sphere   = { builder.createPerFrameBuffer("SceneSphereBuffer",   16 * sizeof(GpuSphere),   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.plane    = { builder.createPerFrameBuffer("ScenePlaneBuffer",    16 * sizeof(GpuPlane),    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.box      = { builder.createPerFrameBuffer("SceneBoxBuffer",      16 * sizeof(GpuBox),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.quad     = { builder.createPerFrameBuffer("SceneQuadBuffer",     16 * sizeof(GpuQuad),     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.vertex   = { builder.createPerFrameBuffer("SceneVertexBuffer",   16 * sizeof(Vertex),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.index    = { builder.createPerFrameBuffer("SceneIndexBuffer",    16 * sizeof(uint32_t),    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.bvh      = { builder.createPerFrameBuffer("SceneBvhBuffer",      16 * sizeof(GpuBvhNode),  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.mesh     = { builder.createPerFrameBuffer("SceneMeshBuffer",     16 * sizeof(GpuMesh),     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.material = { builder.createPerFrameBuffer("SceneMaterialBuffer", 16 * sizeof(GpuMaterial), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.object   = { builder.createPerFrameBuffer("SceneObjectBuffer",   sizeof(GpuObjectHeader) + 16 * sizeof(ObjectHandle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.light    = { builder.createPerFrameBuffer("SceneLightBuffer",    sizeof(GpuLightHeader)  + 16 * sizeof(GpuLight),      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };

    // Pathtracing pass
    ComputePassBuilder pathtrace = builder.addComputePass("PathtracingPass");
    pathtracePassHandle = pathtrace.getHandle();
    pathtrace.setGroup(coreGroupHandle);
    pathtrace.readBuffer( 0, pathtracingUBOHandle, BufferUsageType::Uniform);
    pathtrace.readImage ( 1, previousPathtracingImageHandle, ImageUsageType::Sampled);
    pathtrace.readBuffer( 2, resources.pixelInfoBufferHandle,        BufferUsageType::Storage);
    pathtrace.readBuffer( 3, resources.sceneHandles.sphere.handle,   BufferUsageType::Storage);
    pathtrace.readBuffer( 4, resources.sceneHandles.plane.handle,    BufferUsageType::Storage);
    pathtrace.readBuffer( 5, resources.sceneHandles.box.handle,      BufferUsageType::Storage);
    pathtrace.readBuffer( 6, resources.sceneHandles.vertex.handle,   BufferUsageType::Storage);
    pathtrace.readBuffer( 7, resources.sceneHandles.index.handle,    BufferUsageType::Storage);
    pathtrace.readBuffer( 8, resources.sceneHandles.bvh.handle,      BufferUsageType::Storage);
    pathtrace.readBuffer( 9, resources.sceneHandles.mesh.handle,     BufferUsageType::Storage);
    pathtrace.readBuffer(10, resources.sceneHandles.material.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(11, resources.sceneHandles.object.handle,   BufferUsageType::Storage);
    pathtrace.readBuffer(12, resources.sceneHandles.light.handle,    BufferUsageType::Storage);
    pathtrace.readBuffer(13, resources.sceneHandles.quad.handle,     BufferUsageType::Storage);
    pathtrace.writeImage(14, currentPathtracingImageHandle,  ImageUsageType::Storage);
    pathtrace.readImage (15, lensImageHandle,                ImageUsageType::Sampled);
    pathtrace.setPipeline("./src/shaders/core/pathtracing.glsl");
    pathtracingTimestamp = pathtrace.setTimestamp();

    // Compositing pass
    ComputePassBuilder composite = builder.addComputePass("CompositionPass");
    compositePassHandle = composite.getHandle();
    composite.setGroup(coreGroupHandle);
    composite.readImage (0, currentPathtracingImageHandle, ImageUsageType::Sampled);
    composite.readBuffer(1, compositingUBOHandle, BufferUsageType::Uniform);
    composite.readBuffer(2, resources.pixelInfoBufferHandle, BufferUsageType::Storage);
    composite.writeImage(3, resources.outputImageHandle, ImageUsageType::Storage);
    composite.setPipeline("./src/shaders/core/compositing.glsl");
    compositingTimestamp = composite.setTimestamp();

    TransferPassBuilder exportPass = builder.addTransferPass("ExportPass");
    exportPassHandle = exportPass.getHandle();
    exportPass.setGroup(coreGroupHandle);
    exportPass.copyFrom(resources.outputImageHandle);

    exportService.init(
        engine,
        engine.getExtent().width, engine.getExtent().height,
        resources.pixelInfoBufferHandle
    );

    return resources;
}

void SceneRenderer::destroy() {
    VkSmol& engine = Core::getEngine();
    exportService.destroy(engine);
}

void SceneRenderer::buildPipelines() {
    VkSmol& engine = Core::getEngine();
    engine.waitIdle();

    try {
        engine.reloadPipelines();
    } catch (const std::exception& e) {
        Log::error(e.what());
        return;
    }

    Log::success("SceneRenderer", "(Re)Built the pipelines");
}

void SceneRenderer::render(const FrameContext& frameContext, const Camera& camera) {
    VkSmol& engine = Core::getEngine();

    const bool converged = isRenderFinished();

    if (!converged) {
        pathtracerUBO.sampleCount = accumulator.increment();
        engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);
    }

    const VkExtent2D extent = renderExtent.width > 0 ? renderExtent : frameContext.extent;
    pathtracerUBO.screen.size   = { static_cast<float>(extent.width), static_cast<float>(extent.height) };
    pathtracerUBO.screen.aspect = pathtracerUBO.screen.size.x / pathtracerUBO.screen.size.y;

    const glm::vec3 dir   = camera.getDirection();
    const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(right, dir);
    const float tanHFov   = camera.getTanHFov();
    const float aspect    = pathtracerUBO.screen.aspect;

    pathtracerUBO.camera.eye          = camera.getPosition();
    pathtracerUBO.camera.U            = right * aspect * tanHFov;
    pathtracerUBO.camera.V            = camUp * tanHFov;
    pathtracerUBO.camera.W            = dir;
    pathtracerUBO.camera.thinLens.lensRadius = camera.getLensRadius();
    pathtracerUBO.camera.thinLens.focusDistance = camera.getFocusDistance();

    const TiltShiftState ts = camera.getTiltShift();
    pathtracerUBO.camera.tiltShift.focusA  = ts.focusA;
    pathtracerUBO.camera.tiltShift.focusB  = ts.focusB;
    pathtracerUBO.camera.tiltShift.focusC  = ts.focusC;
    pathtracerUBO.camera.tiltShift.enabled = ts.enabled ? 1 : 0;

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle, frameContext.currentFrame), &pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(compositingUBOHandle, frameContext.currentFrame), &compositingUBO);

    CommandBuffer& commandBuffer = engine.beginRecording(coreGroupHandle);

    if (!converged)
        engine.dispatch(commandBuffer, pathtracePassHandle, (extent.width + 7) / 8, (extent.height + 7) / 8);

    engine.dispatch(commandBuffer, compositePassHandle, (extent.width + 7) / 8, (extent.height + 7) / 8);

    engine.emitBarriers(commandBuffer, exportPassHandle);

    engine.endRecording(coreGroupHandle);
}

void SceneRenderer::saveCapture(const std::filesystem::path& path) {
    VkSmol& engine = Core::getEngine();
    exportService.save(engine, engine.getImage(resources.outputImageHandle), path, aovFlags);
}

void SceneRenderer::resize(uint32_t width, uint32_t height) {
    VkSmol& engine = Core::getEngine();
    engine.waitIdle();
    if (engine.isHeadless())
        engine.getExtent() = { width, height };
    renderExtent = { width, height };

    engine.resizeImage(previousPathtracingImageHandle, width, height);
    engine.resizeImage(currentPathtracingImageHandle, width, height);
    engine.resizeImage(resources.outputImageHandle, width, height);
    engine.resizeBuffer(resources.pixelInfoBufferHandle, static_cast<size_t>(width) * height * sizeof(PixelInfo));
    exportService.resize(engine, width, height);
}


void SceneRenderer::bindParameters() {
    ParameterRegistry& parameters = Core::getParameters();
    parameters.bind<bool>("renderer/denoising", [this](bool v) {
        compositingUBO.denoisingEnabled = static_cast<int>(v);
    });

    parameters.bind("renderer/sampling/max_bounces",      &pathtracerUBO.render.maxBounces);
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
