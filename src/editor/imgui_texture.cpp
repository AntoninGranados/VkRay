#include "imgui_texture.hpp"

#include "imgui/imgui_impl_vulkan.h"

namespace ui {

ImGuiTexture::ImGuiTexture(VkSampler sampler, VkImageView view, VkImageLayout layout)
    : id((ImTextureID)ImGui_ImplVulkan_AddTexture(sampler, view, layout)) {}

ImGuiTexture::ImGuiTexture(ImGuiTexture&& other) noexcept : id(other.id) {
    other.id = 0;
}

ImGuiTexture& ImGuiTexture::operator=(ImGuiTexture&& other) noexcept {
    if (this != &other) {
        reset();
        id = other.id;
        other.id = 0;
    }
    return *this;
}

ImGuiTexture::~ImGuiTexture() {
    reset();
}

void ImGuiTexture::reset() {
    if (id) ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)id);
    id = 0;
}

}
