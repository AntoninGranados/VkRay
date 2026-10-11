#include "pathtrace_renderer.hpp"

#include <algorithm>
#include <string>

#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/fields/parameters.hpp"
#include "core/render/compositing_table.hpp"
#include "core/scene/gpu_structs.hpp"

RenderResources PathtraceRenderer::initGraph(RenderGraphBuilder& builder, VkExtent2D extent, const std::string& tag,
                                             ImageHandle lensImageHandle, BufferHandle blueNoiseBufferHandle) {
    renderExtent = extent;
    groupHandle = builder.addSubmissionGroup(tag.empty() ? "Core" : tag);

    previousPathtracingImageHandle =
        builder.createImage(tag + "PreviousPathtracingImage", VK_FORMAT_R32G32B32A32_SFLOAT, extent.width,
                            extent.height, 1, VKSMOL_IMAGE_OWNERSHIP_MANAGED, VK_IMAGE_USAGE_STORAGE_BIT,
                            ImageAccessInfo{.usage = ImageUsageType::Sampled, .access = AccessType::Read},
                            ImageAccessInfo{.usage = ImageUsageType::Sampled, .access = AccessType::Read});
    currentPathtracingImageHandle = builder.createImage(tag + "CurrentPathtracingImage", VK_FORMAT_R32G32B32A32_SFLOAT,
                                                        extent.width, extent.height);
    resources.outputImageHandle =
        builder.createImage(tag + "OutputImage", VK_FORMAT_R32G32B32A32_SFLOAT, extent.width, extent.height);

    pathtracingUBOHandle = builder.createBuffer(tag + "PathtracingUBO", sizeof(PathtracerUBO));
    resources.pixelInfoBufferHandle = builder.createBuffer(
        tag + "PixelInfoBuffer", static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo));

    // Scene buffers must be created before passes so binding slots can be declared
    resources.sceneHandles.vertex = builder.createBuffer(tag + "SceneVertexBuffer", 16 * sizeof(Vertex));
    resources.sceneHandles.index = builder.createBuffer(tag + "SceneIndexBuffer", 16 * sizeof(uint32_t));
    resources.sceneHandles.bvh = builder.createBuffer(tag + "SceneBvhBuffer", 16 * sizeof(GpuBvhNode));
    resources.sceneHandles.mesh = builder.createBuffer(tag + "SceneMeshBuffer", 16 * sizeof(GpuMesh));
    resources.sceneHandles.material = builder.createBuffer(tag + "SceneMaterialBuffer", 16 * sizeof(GpuMaterial));
    resources.sceneHandles.pluginParams = builder.createBuffer(tag + "ScenePluginParamsBuffer", 16 * sizeof(float));
    resources.sceneHandles.object =
        builder.createBuffer(tag + "SceneObjectBuffer", sizeof(GpuObjectHeader) + 16 * sizeof(GpuObject));
    resources.sceneHandles.light =
        builder.createBuffer(tag + "SceneLightBuffer", sizeof(GpuLightHeader) + 16 * sizeof(GpuLight));
    resources.sceneHandles.motion = builder.createBuffer(tag + "SceneMotionBuffer", 16 * sizeof(GpuMotionSample));
    resources.sceneHandles.liveMotion =
        builder.createBuffer(tag + "SceneLiveMotionBuffer", 16 * sizeof(GpuMotionSample));

    // Pathtracing pass
    ComputePassBuilder pathtrace = builder.addComputePass(tag + "PathtracingPass");
    pathtracePassHandle = pathtrace.getHandle();
    pathtrace.setGroup(groupHandle);
    const std::string pathtracingShader = "builtin:/shaders/core/pathtracing.slang";
    const ShaderDefines pathtracingDefines = {{"RANDOM_BLUE_NOISE", "1"}, {"CAMERA_LENS_APERTURE", "1"}};
    const auto pathtracingLocation = [&](const std::string& name) {
        return Core::getEngine().getShaderLocation(pathtracingShader, name, pathtracingDefines);
    };
    pathtrace.readBuffer(pathtracingLocation("ubo"), pathtracingUBOHandle, BufferUsageType::Uniform);
    pathtrace.readImage(pathtracingLocation("prevTex"), previousPathtracingImageHandle, ImageUsageType::Sampled);
    pathtrace.readBuffer(pathtracingLocation("pixelInfoBuffer"), resources.pixelInfoBufferHandle,
                         BufferUsageType::Storage);
    pathtrace.writeImage(pathtracingLocation("outputImage"), currentPathtracingImageHandle, ImageUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.vertices"), resources.sceneHandles.vertex,
                         BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.indices"), resources.sceneHandles.index, BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.bvhNodes"), resources.sceneHandles.bvh, BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.meshes"), resources.sceneHandles.mesh, BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.materials"), resources.sceneHandles.material,
                         BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.pluginParams"), resources.sceneHandles.pluginParams,
                         BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.objects"), resources.sceneHandles.object, BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("scene.motion"), resources.sceneHandles.motion, BufferUsageType::Storage);
    pathtrace.readBuffer(pathtracingLocation("lightBuffer"), resources.sceneHandles.light, BufferUsageType::Storage);
    pathtrace.readImage(pathtracingLocation("lensSampler"), lensImageHandle, ImageUsageType::Sampled);
    pathtrace.readBuffer(pathtracingLocation("blueNoiseBuffer"), blueNoiseBufferHandle, BufferUsageType::Storage);
    pathtrace.setPipeline(pathtracingShader, pathtracingDefines);
    pathtracingTimestamp = pathtrace.setTimestamp();

    // Compositing pass
    const size_t passCount = std::max<size_t>(1, CompositingTable::totalDispatchCount(scene.getRegistry()));

    for (size_t i = 0; i < compositingPingHandles.size(); i++) {
        compositingPingHandles[i] = builder.createImage(
            tag + "CompositingPingImage" + std::to_string(i), VK_FORMAT_R32G32B32A32_SFLOAT, extent.width,
            extent.height, 1, VKSMOL_IMAGE_OWNERSHIP_MANAGED, VK_IMAGE_USAGE_STORAGE_BIT,
            ImageAccessInfo{.usage = ImageUsageType::Sampled, .access = AccessType::Read},
            ImageAccessInfo{.usage = ImageUsageType::Sampled, .access = AccessType::Read});
    }

    compositingPassHandles.clear();
    compositingPassUBOHandles.clear();
    const std::string compositingShader = "builtin:/shaders/core/compositing.slang";
    const auto compositingLocation = [&](const std::string& name) {
        return Core::getEngine().getShaderLocation(compositingShader, name);
    };
    for (size_t i = 0; i < passCount; i++) {
        BufferHandle passUboHandle =
            builder.createBuffer(tag + "CompositingPassUBO" + std::to_string(i), sizeof(CompositingPassUBO));
        compositingPassUBOHandles.push_back(passUboHandle);

        const ImageHandle readHandle = i == 0 ? currentPathtracingImageHandle : compositingPingHandles[(i - 1) % 2];
        const ImageHandle writeHandle =
            i == passCount - 1 ? resources.outputImageHandle : compositingPingHandles[i % 2];

        ComputePassBuilder pass = builder.addComputePass(tag + "CompositingPass" + std::to_string(i));
        compositingPassHandles.push_back(pass.getHandle());
        pass.setGroup(groupHandle);
        pass.readImage(compositingLocation("sourceTexture"), readHandle, ImageUsageType::Sampled);
        pass.readBuffer(compositingLocation("passUbo"), passUboHandle, BufferUsageType::Uniform);
        pass.readBuffer(compositingLocation("pixelInfoBuffer"), resources.pixelInfoBufferHandle,
                        BufferUsageType::Storage);
        pass.writeImage(compositingLocation("destinationImage"), writeHandle, ImageUsageType::Storage);
        pass.readBuffer(compositingLocation("pluginParams"), resources.sceneHandles.pluginParams,
                        BufferUsageType::Storage);
        pass.readImage(compositingLocation("originalTexture"), currentPathtracingImageHandle, ImageUsageType::Sampled);
        pass.setPipeline(compositingShader);
        if (i == 0) compositingTimestamp = pass.setTimestamp();
    }

    setDefaultUBOs();

    return resources;
}

void PathtraceRenderer::setDefaultUBOs() {
    ParameterRegistry& parameters = Core::getParameters();

    pathtracerUBO.render.maxBounces = parameters.get<int>("renderer/sampling/max_bounces");
    pathtracerUBO.render.importanceSampling = parameters.get<bool>("renderer/sampling/importance_sampling");
    pathtracerUBO.render.clipAccumulation = parameters.get<bool>("renderer/sampling/clamp");
    pathtracerUBO.render.clipThreshold = parameters.get<float>("renderer/sampling/clamp_threshold");
    pathtracerUBO.render.varianceSampling = parameters.get<bool>("renderer/sampling/adaptive_sampling");
    pathtracerUBO.render.varianceWarmupSamples = parameters.get<int>("renderer/sampling/adaptive_warmup");
}

void PathtraceRenderer::render() {
    VkSmol& engine = Core::getEngine();

    ecs::Registry& registry = scene.getRegistry();
    const ecs::Entity camera = *scene.getActiveCamera();

    const CompositingChainInfo& compositingInfo = registry.ctx().get<CompositingChainInfo>();
    if (std::max<size_t>(1, compositingInfo.passes.size()) != compositingPassHandles.size())
        Core::requestGraphRebuild();

    if (registry.hasChangedSince(seenChangeTick, {&ecs::Compositing})) accumulator.restart();
    seenChangeTick = registry.getChangeTick();

    const bool converged = isRenderFinished();

    if (!converged) {
        pathtracerUBO.sampleCount = accumulator.increment();
        engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);
    }

    pathtracerUBO.screen.size = {static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height)};
    pathtracerUBO.screen.aspect = pathtracerUBO.screen.size.x / pathtracerUBO.screen.size.y;
    pathtracerUBO.camera = buildCameraUBO(registry, camera, pathtracerUBO.screen.aspect);
    pathtracerUBO.render.skyParamsBase = registry.ctx().get<SkyPluginInfo>().paramsBase;
    const CameraMediaInfo& cameraMedia = registry.ctx().get<CameraMediaInfo>();
    pathtracerUBO.render.cameraMediumCount = cameraMedia.count;
    pathtracerUBO.render.cameraMedia = cameraMedia.objects;

    engine.writeBuffer(pathtracingUBOHandle, pathtracerUBO);

    for (size_t i = 0; i < compositingPassUBOHandles.size(); i++) {
        CompositingPassUBO passUbo{};
        if (i < compositingInfo.passes.size()) {
            passUbo.slot = compositingInfo.passes[i].slot;
            passUbo.paramsBase = compositingInfo.passes[i].paramsBase;
            passUbo.passId = compositingInfo.passes[i].passId;
        }
        engine.writeBuffer(compositingPassUBOHandles[i], passUbo);
    }

    CommandBuffer& commandBuffer = engine.beginRecording(groupHandle);

    if (!converged)
        engine.dispatch(commandBuffer, pathtracePassHandle, (renderExtent.width + 7) / 8,
                        (renderExtent.height + 7) / 8);

    for (size_t i = 0; i < compositingPassHandles.size(); i++)
        engine.dispatch(commandBuffer, compositingPassHandles[i], (renderExtent.width + 7) / 8,
                        (renderExtent.height + 7) / 8);

    onAfterDispatch(commandBuffer);

    engine.endRecording(groupHandle);
}

void PathtraceRenderer::resize(uint32_t width, uint32_t height) {
    VkSmol& engine = Core::getEngine();
    engine.waitIdle();
    renderExtent = {width, height};

    engine.resizeImage(previousPathtracingImageHandle, width, height);
    engine.resizeImage(currentPathtracingImageHandle, width, height);
    engine.resizeImage(resources.outputImageHandle, width, height);
    for (const ImageHandle& pingHandle : compositingPingHandles) engine.resizeImage(pingHandle, width, height);
    engine.resizeBuffer(resources.pixelInfoBufferHandle, static_cast<size_t>(width) * height * sizeof(PixelInfo));

    onResize(width, height);
}
