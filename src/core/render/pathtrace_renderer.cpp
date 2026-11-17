#include "pathtrace_renderer.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "VkSmol/graph/pass/compute_pass_builder.hpp"
#include "VkSmol/graph/pass_context.hpp"

#include "core/camera/camera.hpp"
#include "core/core.hpp"
#include "core/ecs/entity.hpp"
#include "core/fields/parameters.hpp"
#include "core/render/script_table.hpp"
#include "core/scene/gpu_structs.hpp"

void PathtraceRenderer::init(VkExtent2D extent, const std::string& passTag, VkSmol::ImageHandle lensImage,
                             VkSmol::BufferHandle blueNoiseBuffer) {
    VkSmol::Engine& engine = Core::getEngine();
    tag = passTag;
    renderExtent = extent;
    lensImageHandle = lensImage;
    blueNoiseBufferHandle = blueNoiseBuffer;

    previousPathtracingImageHandle = engine.createImage(tag + "PreviousPathtracingImage", VK_FORMAT_R32G32B32A32_SFLOAT,
                                                        extent.width, extent.height);
    currentPathtracingImageHandle =
        engine.createImage(tag + "CurrentPathtracingImage", VK_FORMAT_R32G32B32A32_SFLOAT, extent.width, extent.height);
    resources.outputImageHandle =
        engine.createImage(tag + "OutputImage", VK_FORMAT_R32G32B32A32_SFLOAT, extent.width, extent.height);
    for (size_t i = 0; i < compositingPingHandles.size(); i++)
        compositingPingHandles[i] = engine.createImage(tag + "CompositingPingImage" + std::to_string(i),
                                                       VK_FORMAT_R32G32B32A32_SFLOAT, extent.width, extent.height);

    pathtracingUBOHandle = engine.createBuffer(tag + "PathtracingUBO", sizeof(PathtracerUBO));
    resources.pixelInfoBufferHandle = engine.createBuffer(
        tag + "PixelInfoBuffer", static_cast<size_t>(extent.width) * extent.height * sizeof(PixelInfo));

    resources.sceneHandles.vertex = engine.createBuffer(tag + "SceneVertexBuffer", 16 * sizeof(Vertex));
    resources.sceneHandles.index = engine.createBuffer(tag + "SceneIndexBuffer", 16 * sizeof(uint32_t));
    resources.sceneHandles.bvh = engine.createBuffer(tag + "SceneBvhBuffer", 16 * sizeof(GpuBvhNode));
    resources.sceneHandles.mesh = engine.createBuffer(tag + "SceneMeshBuffer", 16 * sizeof(GpuMesh));
    resources.sceneHandles.material = engine.createBuffer(tag + "SceneMaterialBuffer", 16 * sizeof(GpuMaterial));
    resources.sceneHandles.pluginParams = engine.createBuffer(tag + "ScenePluginParamsBuffer", 16 * sizeof(float));
    resources.sceneHandles.object =
        engine.createBuffer(tag + "SceneObjectBuffer", sizeof(GpuObjectHeader) + 16 * sizeof(GpuObject));
    resources.sceneHandles.light =
        engine.createBuffer(tag + "SceneLightBuffer", sizeof(GpuLightHeader) + 16 * sizeof(GpuLight));
    resources.sceneHandles.motion = engine.createBuffer(tag + "SceneMotionBuffer", 16 * sizeof(GpuMotionSample));
    resources.sceneHandles.liveMotion =
        engine.createBuffer(tag + "SceneLiveMotionBuffer", 16 * sizeof(GpuMotionSample));
    const std::array<uint8_t, 4> white = {255, 255, 255, 255};
    const VkSmol::ImageHandle blankTexture = engine.createImage(tag + "BlankTexture", VK_FORMAT_R8G8B8A8_UNORM, 1, 1);
    engine.writeImage(blankTexture, white);
    resources.sceneHandles.textures.assign(kMaxTextures, blankTexture);
    scene.init();

    packingScheduler.add(std::make_unique<ecs::TexturePackingSystem>(scene, resources.sceneHandles, engine));
    packingScheduler.add(std::make_unique<ecs::MaterialPackingSystem>(scene, resources.sceneHandles, engine));
    packingScheduler.add(std::make_unique<ecs::MeshPackingSystem>(scene, resources.sceneHandles, engine));
    packingScheduler.add(std::make_unique<ecs::MeshInstancePackingSystem>(scene, resources.sceneHandles, engine));
    packingScheduler.add(std::make_unique<ecs::ObjectPackingSystem>(scene, resources.sceneHandles, engine));
    packingScheduler.add(std::make_unique<ecs::LightPackingSystem>(scene, resources.sceneHandles, engine));
}

void PathtraceRenderer::update() {
    packingScheduler.run(scene.getRegistry());
    if (!resources.sceneHandles.texturesChanged) return;

    resources.sceneHandles.texturesChanged = false;
    Core::requestGraphRebuild();
}

void PathtraceRenderer::addPasses(VkSmol::RenderGraph& graph) {
    SceneGpuBuffers& sceneHandles = resources.sceneHandles;
    for (const VkSmol::ImageHandle texture : sceneHandles.retiredTextures) Core::getEngine().destroyImage(texture);
    sceneHandles.retiredTextures.clear();

    graph.addComputePass(getPathtracingPassName())
        .pipeline("builtin:/shaders/core/pathtracing.slang",
                  {{"RANDOM_BLUE_NOISE", "1"}, {"CAMERA_LENS_APERTURE", "1"}},
                  ScriptTable::conformances({&ScriptTable::kMaterial, &ScriptTable::kSky, &ScriptTable::kLens}))
        .read("ubo", pathtracingUBOHandle)
        .read("prevTex", previousPathtracingImageHandle)
        .write("pixelInfoBuffer", resources.pixelInfoBufferHandle)
        .write("outputImage", currentPathtracingImageHandle)
        .read("scene.vertices", sceneHandles.vertex)
        .read("scene.indices", sceneHandles.index)
        .read("scene.bvhNodes", sceneHandles.bvh)
        .read("scene.meshes", sceneHandles.mesh)
        .read("scene.materials", sceneHandles.material)
        .read("scene.pluginParams", sceneHandles.pluginParams)
        .read("scene.objects", sceneHandles.object)
        .read("scene.motion", sceneHandles.motion)
        .read("scene.textures", sceneHandles.textures)
        .read("lightBuffer", sceneHandles.light)
        .read("lensSampler", lensImageHandle)
        .read("blueNoiseBuffer", blueNoiseBufferHandle)
        .execute([this](VkSmol::PassContext& context) {
            if (dispatchPathtracing) context.dispatchThreads(renderExtent.width, renderExtent.height);
        });

    const size_t passCount = std::max<size_t>(1, ScriptTable::compositingDispatchCount(scene));
    const VkSmol::ShaderConformances compositingConformances = ScriptTable::conformances({&ScriptTable::kCompositing});

    compositingPassUBOHandles.clear();
    for (size_t i = 0; i < passCount; i++) {
        const VkSmol::BufferHandle passUboHandle =
            graph.createBuffer(tag + "CompositingPassUBO" + std::to_string(i), sizeof(CompositingPassUBO));
        compositingPassUBOHandles.push_back(passUboHandle);

        const VkSmol::ImageHandle readHandle =
            i == 0 ? currentPathtracingImageHandle : compositingPingHandles[(i - 1) % 2];
        const VkSmol::ImageHandle writeHandle =
            i == passCount - 1 ? resources.outputImageHandle : compositingPingHandles[i % 2];

        graph.addComputePass(tag + "CompositingPass" + std::to_string(i))
            .pipeline("builtin:/shaders/core/compositing.slang", {}, compositingConformances)
            .read("sourceTexture", readHandle)
            .read("passUbo", passUboHandle)
            .read("pixelInfoBuffer", resources.pixelInfoBufferHandle)
            .write("destinationImage", writeHandle)
            .read("pluginParams", sceneHandles.pluginParams)
            .read("originalTexture", currentPathtracingImageHandle)
            .execute([this](VkSmol::PassContext& context) {
                context.dispatchThreads(renderExtent.width, renderExtent.height);
            });
    }
}

void PathtraceRenderer::prepare() {
    VkSmol::Engine& engine = Core::getEngine();

    ecs::Registry& registry = scene.getRegistry();
    const ecs::Entity camera = *scene.getActiveCamera();

    const PackingInfo& packingInfo = scene.getPackingInfo();
    if (std::max<size_t>(1, packingInfo.compositingPasses.size()) != compositingPassUBOHandles.size())
        Core::requestGraphRebuild();

    if (registry.hasChangedSince(seenChangeTick, {&ecs::Compositing})) accumulator.restart();
    seenChangeTick = registry.getChangeTick();

    dispatchPathtracing = !isRenderFinished();
    if (dispatchPathtracing) {
        pathtracerUBO.sampleCount = accumulator.increment();
        engine.swapImages(currentPathtracingImageHandle, previousPathtracingImageHandle);
    }

    pathtracerUBO.screen.size = {static_cast<float>(renderExtent.width), static_cast<float>(renderExtent.height)};
    pathtracerUBO.screen.aspect = pathtracerUBO.screen.size.x / pathtracerUBO.screen.size.y;
    pathtracerUBO.camera = buildCameraUBO(scene, camera, pathtracerUBO.screen.aspect, frustumScale);
    pathtracerUBO.render.skySlot = packingInfo.sky.slot;
    pathtracerUBO.render.skyParamsBase = packingInfo.sky.paramsBase;
    pathtracerUBO.render.cameraMediumCount = packingInfo.cameraMedia.count;
    pathtracerUBO.render.cameraMedia = packingInfo.cameraMedia.objects;

    ParameterRegistry& parameters = Core::getParameters();
    pathtracerUBO.render.maxBounces = parameters.get<int>("renderer/sampling/max_bounces");
    pathtracerUBO.render.importanceSampling = parameters.get<bool>("renderer/sampling/importance_sampling");
    pathtracerUBO.render.clipAccumulation = parameters.get<bool>("renderer/sampling/clamp");
    pathtracerUBO.render.clipThreshold = parameters.get<float>("renderer/sampling/clamp_threshold");
    pathtracerUBO.render.varianceSampling = parameters.get<bool>("renderer/sampling/adaptive_sampling");
    pathtracerUBO.render.varianceWarmupSamples = parameters.get<int>("renderer/sampling/adaptive_warmup");

    engine.writeBuffer(pathtracingUBOHandle, pathtracerUBO);

    for (size_t i = 0; i < compositingPassUBOHandles.size(); i++) {
        CompositingPassUBO passUbo{};
        if (i < packingInfo.compositingPasses.size()) {
            passUbo.slot = packingInfo.compositingPasses[i].slot;
            passUbo.paramsBase = packingInfo.compositingPasses[i].paramsBase;
            passUbo.passId = packingInfo.compositingPasses[i].passId;
        }
        engine.writeBuffer(compositingPassUBOHandles[i], passUbo);
    }
}

void PathtraceRenderer::resize(uint32_t width, uint32_t height) {
    VkSmol::Engine& engine = Core::getEngine();
    engine.waitIdle();
    renderExtent = {width, height};

    engine.resizeImage(previousPathtracingImageHandle, width, height);
    engine.resizeImage(currentPathtracingImageHandle, width, height);
    engine.resizeImage(resources.outputImageHandle, width, height);
    for (const VkSmol::ImageHandle& pingHandle : compositingPingHandles) engine.resizeImage(pingHandle, width, height);
    engine.resizeBuffer(resources.pixelInfoBufferHandle, static_cast<size_t>(width) * height * sizeof(PixelInfo));
}
