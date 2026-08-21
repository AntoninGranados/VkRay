#include "pathtrace_renderer.hpp"

#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include "core/core.hpp"
#include "core/parameters/parameters.hpp"
#include "core/scene/gpu_structs.hpp"

RenderResources PathtraceRenderer::initGraph(RenderGraphBuilder& builder, VkExtent2D extent, const std::string& tag, ImageHandle lensImageHandle) {
    renderExtent = extent;
    groupHandle = builder.addSubmissionGroup(tag.empty() ? "Core" : tag);

    previousPathtracingImageHandle = builder.createImage(
        tag + "PreviousPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent.width, extent.height, 1,
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        { .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        VK_IMAGE_USAGE_STORAGE_BIT
    );
    currentPathtracingImageHandle = builder.createImage(
        tag + "CurrentPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent.width, extent.height
    );
    resources.outputImageHandle = builder.createImage(
        tag + "OutputImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent.width, extent.height
    );

    pathtracingUBOHandle = builder.createPerFrameBuffer(tag + "PathtracingUBO", sizeof(PathtracerUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    compositingUBOHandle = builder.createPerFrameBuffer(tag + "CompositingUBO", sizeof(CompositingUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    resources.pixelInfoBufferHandle = builder.createBuffer(
        tag + "PixelInfoBuffer",
        static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    // Scene buffers must be created before passes so binding slots can be declared
    resources.sceneHandles.sphere = { builder.createPerFrameBuffer(tag + "SceneSphereBuffer", 16 * sizeof(GpuSphere), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.plane = { builder.createPerFrameBuffer(tag + "ScenePlaneBuffer", 16 * sizeof(GpuPlane), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.box = { builder.createPerFrameBuffer(tag + "SceneBoxBuffer", 16 * sizeof(GpuBox), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.quad = { builder.createPerFrameBuffer(tag + "SceneQuadBuffer", 16 * sizeof(GpuQuad), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.vertex = { builder.createPerFrameBuffer(tag + "SceneVertexBuffer", 16 * sizeof(Vertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.index = { builder.createPerFrameBuffer(tag + "SceneIndexBuffer", 16 * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.bvh = { builder.createPerFrameBuffer(tag + "SceneBvhBuffer", 16 * sizeof(GpuBvhNode), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.mesh = { builder.createPerFrameBuffer(tag + "SceneMeshBuffer", 16 * sizeof(GpuMesh), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.material = { builder.createPerFrameBuffer(tag + "SceneMaterialBuffer", 16 * sizeof(GpuMaterial), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.object = { builder.createPerFrameBuffer(tag + "SceneObjectBuffer", sizeof(GpuObjectHeader) + 16 * sizeof(ObjectHandle), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.light = { builder.createPerFrameBuffer(tag + "SceneLightBuffer", sizeof(GpuLightHeader) + 16 * sizeof(GpuLight), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };

    // Pathtracing pass
    ComputePassBuilder pathtrace = builder.addComputePass(tag + "PathtracingPass");
    pathtracePassHandle = pathtrace.getHandle();
    pathtrace.setGroup(groupHandle);
    pathtrace.readBuffer(0, pathtracingUBOHandle, BufferUsageType::Uniform);
    pathtrace.readImage(1, previousPathtracingImageHandle, ImageUsageType::Sampled);
    pathtrace.readBuffer(2, resources.pixelInfoBufferHandle, BufferUsageType::Storage);
    pathtrace.readBuffer(3, resources.sceneHandles.sphere.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(4, resources.sceneHandles.plane.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(5, resources.sceneHandles.box.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(6, resources.sceneHandles.vertex.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(7, resources.sceneHandles.index.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(8, resources.sceneHandles.bvh.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(9, resources.sceneHandles.mesh.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(10, resources.sceneHandles.material.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(11, resources.sceneHandles.object.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(12, resources.sceneHandles.light.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(13, resources.sceneHandles.quad.handle, BufferUsageType::Storage);
    pathtrace.writeImage(14, currentPathtracingImageHandle, ImageUsageType::Storage);
    pathtrace.readImage(15, lensImageHandle, ImageUsageType::Sampled);
    pathtrace.setPipeline("./src/shaders/core/pathtracing.glsl");
    pathtracingTimestamp = pathtrace.setTimestamp();

    // Compositing pass
    ComputePassBuilder composite = builder.addComputePass(tag + "CompositionPass");
    compositePassHandle = composite.getHandle();
    composite.setGroup(groupHandle);
    composite.readImage(0, currentPathtracingImageHandle, ImageUsageType::Sampled);
    composite.readBuffer(1, compositingUBOHandle, BufferUsageType::Uniform);
    composite.readBuffer(2, resources.pixelInfoBufferHandle, BufferUsageType::Storage);
    composite.writeImage(3, resources.outputImageHandle, ImageUsageType::Storage);
    composite.setPipeline("./src/shaders/core/compositing.glsl");
    compositingTimestamp = composite.setTimestamp();

    setDefaultUBOs();

    return resources;
}

void PathtraceRenderer::setDefaultUBOs() {
    ParameterRegistry& parameters = Core::getParameters();

    pathtracerUBO.render.lightMode = parameters.get<LightMode>("scene/light_mode");
    pathtracerUBO.render.maxBounces = parameters.get<int>("renderer/sampling/max_bounces");
    pathtracerUBO.render.importanceSampling = parameters.get<bool>("renderer/sampling/importance_sampling");
    pathtracerUBO.render.clipAccumulation = parameters.get<bool>("renderer/sampling/clamp");
    pathtracerUBO.render.clipThreshold = parameters.get<float>("renderer/sampling/clamp_threshold");
    pathtracerUBO.render.varianceSampling = parameters.get<bool>("renderer/sampling/adaptive_sampling");
    pathtracerUBO.render.varianceWarmupSamples = parameters.get<int>("renderer/sampling/adaptive_warmup");

    compositingUBO.denoisingEnabled = parameters.get<bool>("renderer/denoising");
}

void PathtraceRenderer::render(const FrameContext& frameContext, const Camera& camera) {
    VkSmol& engine = Core::getEngine();

    const bool converged = isRenderFinished();

    if (!converged) {
        pathtracerUBO.sampleCount = accumulator.increment();
        engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);
    }

    pathtracerUBO.screen.size = { static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height) };
    pathtracerUBO.screen.aspect = pathtracerUBO.screen.size.x / pathtracerUBO.screen.size.y;

    const glm::vec3 dir = camera.getDirection();
    const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(right, dir);
    const float tanHFov = camera.getTanHFov();
    const float aspect = pathtracerUBO.screen.aspect;

    pathtracerUBO.camera.eye = camera.getPosition();
    pathtracerUBO.camera.U = right * aspect * tanHFov;
    pathtracerUBO.camera.V = camUp * tanHFov;
    pathtracerUBO.camera.W = dir;
    pathtracerUBO.camera.thinLens.lensRadius = camera.getLensRadius();
    pathtracerUBO.camera.thinLens.focusDistance = camera.getFocusDistance();

    const TiltShiftState ts = camera.getTiltShift();
    pathtracerUBO.camera.tiltShift.focusA = ts.focusA;
    pathtracerUBO.camera.tiltShift.focusB = ts.focusB;
    pathtracerUBO.camera.tiltShift.focusC = ts.focusC;
    pathtracerUBO.camera.tiltShift.enabled = ts.enabled ? 1 : 0;

    engine.fillBuffer(engine.getBuffer(pathtracingUBOHandle, frameContext.currentFrame), &pathtracerUBO);
    engine.fillBuffer(engine.getBuffer(compositingUBOHandle, frameContext.currentFrame), &compositingUBO);

    CommandBuffer& commandBuffer = engine.beginRecording(groupHandle);

    if (!converged)
        engine.dispatch(commandBuffer, pathtracePassHandle, (renderExtent.width + 7) / 8, (renderExtent.height + 7) / 8);

    engine.dispatch(commandBuffer, compositePassHandle, (renderExtent.width + 7) / 8, (renderExtent.height + 7) / 8);

    onAfterDispatch(commandBuffer);

    engine.endRecording(groupHandle);
}

void PathtraceRenderer::resize(uint32_t width, uint32_t height) {
    VkSmol& engine = Core::getEngine();
    engine.waitIdle();
    renderExtent = { width, height };

    engine.resizeImage(previousPathtracingImageHandle, width, height);
    engine.resizeImage(currentPathtracingImageHandle, width, height);
    engine.resizeImage(resources.outputImageHandle, width, height);
    engine.resizeBuffer(resources.pixelInfoBufferHandle, static_cast<size_t>(width) * height * sizeof(PixelInfo));

    onResize(width, height);
}
