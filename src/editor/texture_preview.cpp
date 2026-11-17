#include "texture_preview.hpp"

#include "VkSmol/engine.hpp"

#include "core/core.hpp"

std::optional<ImTextureID> TexturePreview::get(ecs::Entity texture) {
    const PackingInfo& packingInfo = Core::getScene().getPackingInfo();
    if (packingInfo.textureGeneration != generation) {
        cache.clear();
        generation = packingInfo.textureGeneration;
    }

    const auto cached = cache.find(texture);
    if (cached != cache.end()) return cached->second.get();

    const auto slot = packingInfo.textureSlots.find(texture);
    if (slot == packingInfo.textureSlots.end()) return std::nullopt;

    VkSmol::Engine& engine = Core::getEngine();
    const VkSmol::ImageHandle image = Core::getCoreRenderer().getResources().sceneHandles.textures[slot->second];
    const auto inserted = cache.emplace(texture, ui::ImGuiTexture(engine.getSampler(image),
                                                                  engine.getImageView(image, VK_IMAGE_VIEW_TYPE_2D),
                                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    return inserted.first->second.get();
}
