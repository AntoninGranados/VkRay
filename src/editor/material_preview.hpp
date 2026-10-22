#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"
#include "imgui/imgui.h"

#include "core/ecs/entity.hpp"
#include "core/ecs/registry.hpp"
#include "core/fields/field.hpp"
#include "core/render/pathtrace_renderer.hpp"
#include "core/scene/scene.hpp"
#include "editor/imgui_texture.hpp"

struct MaterialFingerprint {
    const ecs::ComponentType* type = nullptr;
    std::vector<FieldValue> fields;
    std::string programmableBody;
    std::vector<float> programmableValues;

    bool operator==(const MaterialFingerprint&) const = default;
};

class MaterialPreview {
public:
    static constexpr int kPreviewSize = 256;
    static constexpr int kPreviewSampleCount = 256;
    static constexpr uint32_t kThumbnailCapacity = 32;

    void init();
    void addPasses(VkSmol::RenderGraph& graph);
    void tick();

    void drawPreview(ecs::Entity materialEntity);

    Scene& getScene() { return renderer.getScene(); }
    VkSmol::ImageHandle getOutputImageHandle() const { return renderer.getOutputImageHandle(); }
    VkSmol::ImageHandle getThumbnailImageHandle() const { return thumbnailImageHandle; }

private:
    struct InFlight {
        ecs::Entity entity;
        MaterialFingerprint fingerprint;
    };

    struct Thumbnail {
        uint32_t layer;
        MaterialFingerprint fingerprint;
    };

    std::optional<ImTextureID> getPreview(ecs::Entity materialEntity);
    const ecs::ComponentType* resolveBsdfType(ecs::Registry& registry, ecs::Entity entity) const;
    ecs::Entity resolveMaterialSource(ecs::Entity entity) const;
    MaterialFingerprint captureFingerprint(ecs::Entity entity) const;
    bool isStale(ecs::Entity entity, const MaterialFingerprint& fingerprint) const;
    ui::ImGuiTexture registerTexture(VkSmol::ImageRange image) const;

    void syncPreviewMaterial(const ecs::ComponentType& type, ecs::Entity fieldSource);
    void startGeneration(ecs::Entity materialEntity);
    void finishGeneration();
    void evictDeadEntries();
    void evict(ecs::Entity materialEntity);

    PathtraceRenderer renderer;
    ecs::Entity previewMaterialEntity;
    ui::ImGuiTexture liveTexture;

    VkSmol::ImageHandle thumbnailImageHandle;
    std::vector<ui::ImGuiTexture> thumbnailTextures;
    std::vector<uint32_t> freeLayers;
    std::optional<uint32_t> pendingCopyLayer;

    std::optional<InFlight> inFlight;
    std::deque<ecs::Entity> pendingMaterials;
    std::unordered_map<ecs::Entity, Thumbnail> thumbnails;
    std::deque<ecs::Entity> thumbnailOrder;
};
