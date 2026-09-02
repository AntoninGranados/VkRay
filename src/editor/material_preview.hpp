#pragma once

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "imgui/imgui.h"

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/field.hpp"
#include "core/render/pathtrace_renderer.hpp"
#include "core/scene/scene.hpp"

struct FrameContext;

struct MaterialFingerprint {
    const ecs::ComponentType* type = nullptr;
    std::vector<FieldValue> fields;
    std::string programmableBody;
    std::vector<float> programmableValues;

    bool operator==(const MaterialFingerprint&) const = default;
};

struct PreviewImage {
    Image image;
    ImageView view;
    ImTextureID textureId;
    MaterialFingerprint fingerprint;
};

class MaterialPreview {
public:
    static constexpr int kPreviewSize = 256;
    static constexpr int kPreviewSampleCount = 256;

    RenderResources initGraph(RenderGraphBuilder& builder, ImageHandle lensImageHandle);
    void onGraphCompiled(const RenderResources& resources);
    void tick(const FrameContext& frameContext);

    void drawPreview(ecs::Entity materialEntity);

    Scene& getScene() { return renderer.getScene(); }

private:
    struct InFlight {
        ecs::Entity entity;
        MaterialFingerprint fingerprint;
    };

    std::optional<ImTextureID> getPreview(ecs::Entity materialEntity);
    const ecs::ComponentType* resolveBsdfType(ecs::Registry& registry, ecs::Entity entity) const;
    ecs::Entity resolveMaterialSource(ecs::Entity entity) const;
    MaterialFingerprint captureFingerprint(ecs::Entity entity) const;
    bool isStale(ecs::Entity entity, const MaterialFingerprint& fingerprint) const;
    ImTextureID registerTexture(VkImageView view) const;

    void syncPreviewMaterial(const ecs::ComponentType& type, ecs::Entity fieldSource);
    void startGeneration(ecs::Entity materialEntity);
    void finishGeneration();
    void evictDeadEntries();
    void evict(ecs::Entity materialEntity);

    PathtraceRenderer renderer;
    ecs::Entity previewMaterialEntity;
    ImTextureID liveTextureId = 0;

    std::optional<InFlight> inFlight;
    std::deque<ecs::Entity> pendingMaterials;
    std::unordered_map<ecs::Entity, PreviewImage> previewImages;
};
