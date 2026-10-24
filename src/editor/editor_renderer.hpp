#pragma once

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include "editor/imgui_texture.hpp"
#include "editor/render_structures.hpp"
#include "imgui/imgui.h"

class EditorRenderer {
public:
    static constexpr const char* kDisplayPassName = "DisplayPass";
    static constexpr const char* kDebugPassName = "DebugPass";
    static constexpr const char* kUiPassName = "UiPass";

    void init();
    void addPasses(VkSmol::RenderGraph& graph);
    void registerImGuiTextures();
    void resize(VkExtent2D renderExtent, VkExtent2D viewportExtent);
    void prepare();

    ImTextureID getDisplayTexId() const { return displayTex; }
    ImTextureID getDebugTexId() const { return debugTex; }
    ImTextureID getOutputTexId() const { return outputTex; }

private:
    DebugUBO debugUBO{};
    DisplayUBO displayUBO{};

    VkSmol::ImageHandle displayImageHandle;
    VkSmol::ImageHandle debugImageHandle;

    VkSmol::BufferHandle debugUBOHandle;
    VkSmol::BufferHandle displayUBOHandle;

    VkExtent2D renderExtent = {};
    VkExtent2D viewportExtent = {};

    ui::ImGuiTexture displayTex;
    ui::ImGuiTexture debugTex;
    ui::ImGuiTexture outputTex;
};
