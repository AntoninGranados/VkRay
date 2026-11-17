#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "imgui/imgui.h"

#include "core/ecs/entity.hpp"
#include "editor/imgui_texture.hpp"

class TexturePreview {
public:
    std::optional<ImTextureID> get(ecs::Entity texture);

private:
    uint32_t generation = 0;
    std::unordered_map<ecs::Entity, ui::ImGuiTexture> cache;
};
