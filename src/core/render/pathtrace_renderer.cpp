#include "pathtrace_renderer.hpp"

#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/render_graph_builder.hpp"

#include "core/core.hpp"
#include "core/ecs/components/camera.hpp"
#include "core/ecs/components/component.hpp"
#include "core/ecs/components/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/parameters/parameters.hpp"
#include "core/scene/gpu_structs.hpp"
#include <cmath>

RenderResources PathtraceRenderer::initGraph(RenderGraphBuilder& builder, VkExtent2D extent, const std::string& tag, ImageHandle lensImageHandle) {
    renderExtent = extent;
    groupHandle = builder.addSubmissionGroup(tag.empty() ? "Core" : tag);

    previousPathtracingImageHandle = builder.createImage(
        tag + "PreviousPathtracingImage",
        VK_FORMAT_R32G32B32A32_SFLOAT,
        extent.width, extent.height, 1,
        VKSMOL_IMAGE_OWNERSHIP_MANAGED,
        VK_IMAGE_USAGE_STORAGE_BIT,
        ImageAccessInfo{ .usage = ImageUsageType::Sampled, .access = AccessType::Read },
        ImageAccessInfo{ .usage = ImageUsageType::Sampled, .access = AccessType::Read }
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

    pathtracingUBOHandle = builder.createBuffer(tag + "PathtracingUBO", sizeof(PathtracerUBO), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    compositingUBOHandle = builder.createBuffer(tag + "CompositingUBO", sizeof(CompositingUBO), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    resources.pixelInfoBufferHandle = builder.createBuffer(
        tag + "PixelInfoBuffer",
        static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo),
        0,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    // Scene buffers must be created before passes so binding slots can be declared
    resources.sceneHandles.sphere = { builder.createBuffer(tag + "SceneSphereBuffer", 16 * sizeof(GpuSphere), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.plane = { builder.createBuffer(tag + "ScenePlaneBuffer", 16 * sizeof(GpuPlane), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.box = { builder.createBuffer(tag + "SceneBoxBuffer", 16 * sizeof(GpuBox), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.quad = { builder.createBuffer(tag + "SceneQuadBuffer", 16 * sizeof(GpuQuad), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.vertex = { builder.createBuffer(tag + "SceneVertexBuffer", 16 * sizeof(Vertex), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.index = { builder.createBuffer(tag + "SceneIndexBuffer", 16 * sizeof(uint32_t), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.bvh = { builder.createBuffer(tag + "SceneBvhBuffer", 16 * sizeof(GpuBvhNode), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.mesh = { builder.createBuffer(tag + "SceneMeshBuffer", 16 * sizeof(GpuMesh), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.material = { builder.createBuffer(tag + "SceneMaterialBuffer", 16 * sizeof(GpuMaterial), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.object = { builder.createBuffer(tag + "SceneObjectBuffer", sizeof(GpuObjectHeader) + 16 * sizeof(ObjectHandle), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };
    resources.sceneHandles.light = { builder.createBuffer(tag + "SceneLightBuffer", sizeof(GpuLightHeader) + 16 * sizeof(GpuLight), VKSMOL_BUFFER_CREATE_PER_FRAME_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), 16 };

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
    pathtrace.readBuffer(6, resources.sceneHandles.quad.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(7, resources.sceneHandles.vertex.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(8, resources.sceneHandles.index.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(9, resources.sceneHandles.bvh.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(10, resources.sceneHandles.mesh.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(11, resources.sceneHandles.material.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(12, resources.sceneHandles.object.handle, BufferUsageType::Storage);
    pathtrace.readBuffer(13, resources.sceneHandles.light.handle, BufferUsageType::Storage);
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

void PathtraceRenderer::render(const FrameContext& frameContext, const ecs::Registry& registry, const ecs::Entity& camera) {
    VkSmol& engine = Core::getEngine();

    const bool converged = isRenderFinished();

    if (!converged) {
        pathtracerUBO.sampleCount = accumulator.increment();
        engine.swapBindings(currentPathtracingImageHandle, previousPathtracingImageHandle);
    }

    pathtracerUBO.screen.size = { static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height) };
    pathtracerUBO.screen.aspect = pathtracerUBO.screen.size.x / pathtracerUBO.screen.size.y;

    const ecs::Component& t = registry.get(camera, ecs::Transform);

    const glm::vec3 dir = directionFromRotation(t.get<glm::vec3>("rotation"));
    const glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(right, dir);
    const float tanHFov = glm::tan(glm::radians(effectiveFov(registry, camera)) * 0.5f);
    const float aspect = pathtracerUBO.screen.aspect;

    pathtracerUBO.camera.eye = t.get<glm::vec3>("position");
    pathtracerUBO.camera.U = right * aspect * tanHFov;
    pathtracerUBO.camera.V = camUp * tanHFov;
    pathtracerUBO.camera.W = dir;

    const bool hasTL = registry.has(camera, ecs::ThinLens);
    float lensRadius = 0.0f;
    float focusDistance = 10.0f;
    if (hasTL) {
        const ecs::Component& tl = registry.get(camera, ecs::ThinLens);
        const float sensorWidth = Core::getParameters().get<float>("internal/sensor_width");
        lensRadius = lensRadiusFromFStop(tl.get<float>("focal_length") / sensorWidth, tl.get<float>("f_stop"));
        focusDistance = tl.get<float>("focal_distance");
    }
    pathtracerUBO.camera.thinLens.lensRadius = lensRadius;
    pathtracerUBO.camera.thinLens.focusDistance = focusDistance;

    const auto ts = getTiltShiftState(registry, camera);
    pathtracerUBO.camera.tiltShift.enabled = ts.has_value();
    if (ts.has_value()) {
        pathtracerUBO.camera.tiltShift.focusA = ts->focusA;
        pathtracerUBO.camera.tiltShift.focusB = ts->focusB;
        pathtracerUBO.camera.tiltShift.focusC = ts->focusC;
    }

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
