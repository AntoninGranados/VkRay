#include "core_renderer.hpp"

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass/graphics_pass_builder.hpp"
#include "VkSmol/graph/pass/transfer_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"
#include "VkSmol/image/image.hpp"

#include "app/app_context.hpp"
#include "app/log.hpp"
#include "app/animation_handler.hpp"
#include "app/parameter_handler.hpp"
#include "scene/scene.hpp"
#include "core_structures.hpp"

static AOVFlags buildAOVFlags(ParameterHandler& p) {
    return AOVFlags{
        .positionW = p.getBool("pathtracer/aov/position_w"),
        .position  = p.getBool("pathtracer/aov/position"),
        .normalW   = p.getBool("pathtracer/aov/normal_w"),
        .normal    = p.getBool("pathtracer/aov/normal"),
        .albedo    = p.getBool("pathtracer/aov/albedo"),
        .roughness = p.getBool("pathtracer/aov/roughness"),
        .matType   = p.getBool("pathtracer/aov/mat_type"),
        .skyMask   = p.getBool("pathtracer/aov/sky_mask"),
    };
}

CoreResources CoreRenderer::initGraph(VkSmol& engine, RenderGraphBuilder& builder) {
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
    pathtracingPipelineHandle = pathtrace.setPipeline("./src/shaders/pathtracing/pathtracing.glsl");

    // Compositing pass
    ComputePassBuilder composite = builder.addComputePass("CompositionPass");
    compositePassHandle = composite.getHandle();
    composite.setGroup(coreGroupHandle);
    composite.readImage (0, currentPathtracingImageHandle, ImageUsageType::Sampled);
    composite.readBuffer(1, compositingUBOHandle, BufferUsageType::Uniform);
    composite.readBuffer(2, resources.pixelInfoBufferHandle, BufferUsageType::Storage);
    composite.writeImage(3, resources.outputImageHandle, ImageUsageType::Storage);
    compositingPipelineHandle = composite.setPipeline("./src/shaders/compositing.glsl");

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

void CoreRenderer::destroy(VkSmol& engine) {
    exportService.destroy(engine);
}

void CoreRenderer::buildPipelines(VkSmol& engine) {
    engine.waitIdle();

    try {
        engine.reloadPipelines();
    } catch (const std::exception& e) {
        Log::error(e.what());
        return;
    }

    Log::success("CoreRenderer", "(Re)Built the pipelines");
}

void CoreRenderer::render(AppContext& ctx, const FrameContext& frameContext) {
    VkSmol& engine = *ctx.engine;

    bool shouldSave = false;
    std::filesystem::path savePath;
    bool toVideo = false;

    uint64_t renderSamplesPerPixel = ctx.parameters->getInt("pathtracer/sampling/render_samples");
    if (ctx.renderState->renderMode != RenderMode::Preview && renderSamplesPerPixel > 0 && !ctx.renderState->pendingExit && !(*ctx.restartRender)) {
        if (ctx.renderState->sampleCount >= renderSamplesPerPixel) {
            shouldSave = true;

            if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
                savePath = exportService.buildAnimationFramePath(ctx.animation->getFrame());

                ctx.renderState->samplesPerSecEMA          = 0.0;
                ctx.renderState->samplesPerSecInitialized  = false;
                ctx.renderState->samplesPerSecAccumTime    = 0.0;
                ctx.renderState->samplesPerSecAccumSamples = 0.0;

                ctx.animation->stepFixed();
                if (ctx.animation->getFrame() == 0) {
                    ctx.renderState->pendingExit = true;
                    toVideo = true;
                }
            } else {
                savePath = ctx.outputPath;
                ctx.renderState->pendingExit = ctx.renderState->renderMode == RenderMode::RenderSingle;
            }

            if (ctx.renderState->pendingExit) {
                if (onRenderComplete) onRenderComplete();
                ctx.renderState->renderMode                = RenderMode::Preview;
                ctx.renderState->pendingExit               = false;
                ctx.renderState->samplesPerSecEMA          = 0.0;
                ctx.renderState->samplesPerSecInitialized  = false;
                ctx.renderState->samplesPerSecAccumTime    = 0.0;
                ctx.renderState->samplesPerSecAccumSamples = 0.0;
            }

            *ctx.restartRender = true;
        }
    }

    ctx.scene->runOnRender(ctx, frameContext);
    engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle, frameContext.currentFrame), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(compositingUBOHandle, frameContext.currentFrame), ctx.compositingUBO);

    pathtracingPass(ctx, frameContext);

    if (shouldSave) {
        AOVFlags aovFlags = buildAOVFlags(*ctx.parameters);
        exportService.save(engine, engine.getImage(resources.outputImageHandle), savePath, aovFlags);
        if (toVideo) exportService.convertFramesToVideo(ctx.outputPath);
    }
}

void CoreRenderer::renderHeadless(AppContext& ctx, bool captureOutput) {
    VkSmol& engine = *ctx.engine;

    auto frameContext = engine.beginFrame();
    if (!frameContext) return;

    ctx.scene->runPreUpdate(ctx);
    ctx.scene->runOnRender(ctx, *frameContext);
    engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle,  frameContext->currentFrame), ctx.pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(compositingUBOHandle,  frameContext->currentFrame), ctx.compositingUBO);

    pathtracingPass(ctx, *frameContext, captureOutput);

    engine.advanceFrame();
}

void CoreRenderer::handleResize(AppContext& ctx, const VkExtent2D& extent) {
    VkSmol& engine = *ctx.engine;
    engine.waitIdle();

    engine.resizeImage(previousPathtracingImageHandle, extent.width, extent.height);
    engine.resizeImage(currentPathtracingImageHandle, extent.width, extent.height);
    engine.resizeImage(resources.outputImageHandle, extent.width, extent.height);
    engine.resizeBuffer(resources.pixelInfoBufferHandle, static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo));
    exportService.resize(engine, extent.width, extent.height);
}

void CoreRenderer::pathtracingPass(AppContext& ctx, const FrameContext& frameContext, bool captureOutput) {
    VkSmol& engine = *ctx.engine;

    CommandBuffer& commandBuffer = engine.beginRecording(coreGroupHandle);

    VkExtent2D extent = frameContext.extent;

    {   // Pathtrace (compute)
        ComputePipeline& ptPipeline = engine.getComputePipeline(pathtracingPipelineHandle);
        engine.emitBarriers(commandBuffer, pathtracePassHandle);
        engine.bindDescriptors(commandBuffer, pathtracePassHandle);
        ptPipeline.bind(commandBuffer);
        ptPipeline.dispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }

    {   // Compositing (compute)
        ComputePipeline& coPipeline = engine.getComputePipeline(compositingPipelineHandle);
        engine.emitBarriers(commandBuffer, compositePassHandle);
        engine.bindDescriptors(commandBuffer, compositePassHandle);
        coPipeline.bind(commandBuffer);
        coPipeline.dispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8);
    }

    engine.emitBarriers(commandBuffer, exportPassHandle);

    engine.endRecording(coreGroupHandle);
}

void CoreRenderer::saveCapture(AppContext& ctx, const std::filesystem::path& path) {
    AOVFlags aovFlags = buildAOVFlags(*ctx.parameters);
    exportService.save(*ctx.engine, ctx.engine->getImage(resources.outputImageHandle), path, aovFlags);
}

void CoreRenderer::resize(AppContext& ctx, uint32_t width, uint32_t height) {
    ctx.engine->getExtent() = { width, height };
    handleResize(ctx, { width, height });
}
