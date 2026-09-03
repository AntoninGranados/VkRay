#include "material_preview.hpp"

#include <algorithm>

#include "core/ecs/components/material.hpp"
#include "core/scene/scene.hpp"
#include "imgui/imgui_impl_vulkan.h"

#include "VkSmol/graph/render_graph_builder.hpp"

#include "core/core.hpp"
#include "core/render/programmable_shader.hpp"
#include "editor/ui_utils.hpp"

RenderResources MaterialPreview::initGraph(RenderGraphBuilder& builder, ImageHandle lensImageHandle) {
    Scene& scene = renderer.getScene();
    scene.init();

    ecs::Registry& registry = scene.getRegistry();
    registry.get(scene.getDefaultCamera(), ecs::Camera).set<float>("fov", 15.0f);

    previewMaterialEntity = scene.createNamedEntity("Preview Material", scene.getMaterialsRoot());
    registry.add(previewMaterialEntity, ecs::Diffuse);

    const ecs::Entity previewObject = scene.createNamedEntity("Preview Sphere", scene.getObjectsRoot());
    registry.add(previewObject, ecs::Sphere);
    registry.add(previewObject, ecs::MaterialRef);
    registry.get(previewObject, ecs::MaterialRef).set<ecs::Entity>("handle", previewMaterialEntity);

    const ecs::Entity lightMaterialEntity = scene.createNamedEntity("Preview Light Material", scene.getMaterialsRoot());
    registry.add(lightMaterialEntity, ecs::Emissive);
    registry.get(lightMaterialEntity, ecs::Emissive).set<float>("emission_strength", 50.0f);

    const ecs::Entity lightObject = scene.createNamedEntity("Preview Light", scene.getObjectsRoot());
    registry.add(lightObject, ecs::Sphere);
    registry.get(lightObject, ecs::Sphere).set<float>("radius", 1.5f);
    registry.get(lightObject, ecs::Transform).set<glm::vec3>("position", glm::vec3(10, 8, -14));
    registry.add(lightObject, ecs::MaterialRef);
    registry.get(lightObject, ecs::MaterialRef).set<ecs::Entity>("handle", lightMaterialEntity);

    RenderResources resources = renderer.initGraph(builder, VkExtent2D{ kPreviewSize, kPreviewSize }, "MaterialPreview", lensImageHandle);
    renderer.setTargetSampleCount(0);
    renderer.setLightMode(LightMode::Studio);
    return resources;
}

void MaterialPreview::onGraphCompiled(const RenderResources& resources) {
    renderer.getScene().setGpuBufferHandles(resources.sceneHandles);
    liveTextureId = registerTexture(Core::getEngine().getView(renderer.getOutputImageHandle()).get());
}

const ecs::ComponentType* MaterialPreview::resolveBsdfType(ecs::Registry& registry, ecs::Entity entity) const {
    if (registry.has(entity, ecs::Principled)) return &ecs::Principled;
    if (registry.has(entity, ecs::Emissive)) return &ecs::Emissive;
    if (registry.has(entity, ecs::Diffuse)) return &ecs::Diffuse;
    if (registry.has(entity, ecs::Metal)) return &ecs::Metal;
    if (registry.has(entity, ecs::Glossy)) return &ecs::Glossy;
    if (registry.has(entity, ecs::Dielectric)) return &ecs::Dielectric;
    if (registry.has(entity, ecs::Volume)) return &ecs::Volume;
    if (registry.has(entity, ecs::ProgrammableMaterial)) return &ecs::ProgrammableMaterial;
    return nullptr;
}

ecs::Entity MaterialPreview::resolveMaterialSource(ecs::Entity entity) const {
    return resolveBsdfType(Core::getScene().getRegistry(), entity) ? entity : Core::getScene().getDefaultMaterial();
}

MaterialFingerprint MaterialPreview::captureFingerprint(ecs::Entity entity) const {
    ecs::Registry& registry = Core::getScene().getRegistry();
    entity = resolveMaterialSource(entity);

    MaterialFingerprint fingerprint;
    fingerprint.type = resolveBsdfType(registry, entity);
    if (fingerprint.type) {
        const ecs::Component& component = registry.get(entity, *fingerprint.type);
        for (const ecs::ComponentField& field : component.getFields())
            fingerprint.fields.push_back(field);
        if (fingerprint.type == &ecs::ProgrammableMaterial) {
            const ProgrammableShader& shader = component.payload<ProgrammableShader>("shader");
            fingerprint.programmableBody = shader.getMainBody();
            fingerprint.programmableValues = shader.packValues();
        }
    }
    return fingerprint;
}

bool MaterialPreview::isStale(ecs::Entity entity, const MaterialFingerprint& fingerprint) const {
    return captureFingerprint(entity) != fingerprint;
}

ImTextureID MaterialPreview::registerTexture(VkImageView view) const {
    VkSmol& engine = Core::getEngine();
    return (ImTextureID)ImGui_ImplVulkan_AddTexture(
        engine.getSampler(renderer.getOutputImageHandle()).get(),
        view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void MaterialPreview::syncPreviewMaterial(const ecs::ComponentType& type, ecs::Entity fieldSource) {
    ecs::Registry& previewRegistry = renderer.getScene().getRegistry();
    if (!previewRegistry.has(previewMaterialEntity, type)) {
        const ecs::ComponentType* previousType = resolveBsdfType(previewRegistry, previewMaterialEntity);
        if (previousType) {
            previewRegistry.remove(previewMaterialEntity, *previousType);
            previewRegistry.flush();
        }
        previewRegistry.add(previewMaterialEntity, type);
    }

    ecs::Component& source = Core::getScene().getRegistry().get(fieldSource, type);
    ecs::Component& preview = previewRegistry.get(previewMaterialEntity, type);

    for (const ecs::ComponentField& field : source.getFields())
        preview.getField(field.getId()) = field;

    if (type == ecs::ProgrammableMaterial) {
        ProgrammableShader& previewShader = preview.payload<ProgrammableShader>("shader");
        previewShader.parse(source.get<std::filesystem::path>("path"));

        const ProgrammableShader& sourceShader = source.payload<ProgrammableShader>("shader");
        for (const ecs::ComponentField& field : sourceShader.getParams())
            previewShader.getField(field.getId()) = field;
    }
}

void MaterialPreview::startGeneration(ecs::Entity materialEntity) {
    const MaterialFingerprint fingerprint = captureFingerprint(materialEntity);
    inFlight = InFlight{ materialEntity, fingerprint };

    if (fingerprint.type)
        syncPreviewMaterial(*fingerprint.type, resolveMaterialSource(materialEntity));

    renderer.setTargetSampleCount(kPreviewSampleCount);
    renderer.restartAccumulation();
}

void MaterialPreview::finishGeneration() {
    VkSmol& engine = Core::getEngine();
    const VkExtent2D extent = renderer.getRenderExtent();

    auto it = previewImages.find(inFlight->entity);
    if (it == previewImages.end()) {
        PreviewImage preview;
        preview.image = engine.createEmptyImage(VK_FORMAT_R32G32B32A32_SFLOAT, extent.width, extent.height, "MaterialPreviewThumbnail");
        preview.view = engine.createImageView(preview.image);
        preview.textureId = registerTexture(preview.view.get());
        it = previewImages.emplace(inFlight->entity, std::move(preview)).first;
    }

    engine.waitIdle();
    engine.copyImage(engine.getImage(renderer.getOutputImageHandle()), it->second.image);
    it->second.fingerprint = inFlight->fingerprint;

    inFlight.reset();
    renderer.setTargetSampleCount(0);
}

void MaterialPreview::evict(ecs::Entity materialEntity) {
    auto it = previewImages.find(materialEntity);
    if (it == previewImages.end()) return;

    VkSmol& engine = Core::getEngine();
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)it->second.textureId);
    engine.destroyImageView(it->second.view);
    engine.destroyImage(it->second.image);
    previewImages.erase(it);
}

void MaterialPreview::evictDeadEntries() {
    ecs::Registry& registry = Core::getScene().getRegistry();

    for (auto it = previewImages.begin(); it != previewImages.end();) {
        ecs::Entity entity = it->first;
        ++it;
        if (!registry.isAlive(entity)) evict(entity);
    }

    std::erase_if(pendingMaterials, [&](ecs::Entity e) { return !registry.isAlive(e); });

    if (inFlight.has_value() && !registry.isAlive(inFlight->entity)) {
        inFlight.reset();
        renderer.setTargetSampleCount(0);
    }
}

void MaterialPreview::tick(const FrameContext& frameContext) {
    evictDeadEntries();

    if (!inFlight.has_value() && !pendingMaterials.empty()) {
        startGeneration(pendingMaterials.front());
        pendingMaterials.pop_front();
    } else if (inFlight.has_value() && isStale(inFlight->entity, inFlight->fingerprint)) {
        startGeneration(inFlight->entity);
    }

    Scene& scene = renderer.getScene();
    scene.runOnRender(frameContext);
    renderer.render(frameContext);

    if (inFlight.has_value() && renderer.isRenderFinished())
        finishGeneration();
}

std::optional<ImTextureID> MaterialPreview::getPreview(ecs::Entity materialEntity) {
    if (inFlight.has_value() && inFlight->entity == materialEntity) return liveTextureId;

    auto it = previewImages.find(materialEntity);
    if (it != previewImages.end() && !isStale(materialEntity, it->second.fingerprint))
        return it->second.textureId;

    const bool alreadyQueued = std::find(pendingMaterials.begin(), pendingMaterials.end(), materialEntity) != pendingMaterials.end();
    if (!alreadyQueued) pendingMaterials.push_back(materialEntity);
    return std::nullopt;
}

void MaterialPreview::drawPreview(ecs::Entity materialEntity) {
    if (auto preview = getPreview(materialEntity))
        ui::drawResizableImage("##Preview", *preview);
    else
        ImGui::Text("Loading...");
}
