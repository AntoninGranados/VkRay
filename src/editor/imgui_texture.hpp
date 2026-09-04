#pragma once

#include "VkSmol/engine.hpp"
#include "imgui/imgui.h"

namespace ui {

class ImGuiTexture {
public:
    ImGuiTexture() = default;
    ImGuiTexture(VkSampler sampler, VkImageView view, VkImageLayout layout);

    ImGuiTexture(const ImGuiTexture&) = delete;
    ImGuiTexture& operator=(const ImGuiTexture&) = delete;
    ImGuiTexture(ImGuiTexture&& other) noexcept;
    ImGuiTexture& operator=(ImGuiTexture&& other) noexcept;
    ~ImGuiTexture();

    ImTextureID get() const { return id; }
    operator ImTextureID() const { return id; }

private:
    void reset();

    ImTextureID id = 0;
};

}
