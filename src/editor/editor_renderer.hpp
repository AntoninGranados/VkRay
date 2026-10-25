#pragma once

#include "VkSmol/graph/builder_resource.hpp"
#include "VkSmol/graph/render_graph.hpp"

#include "editor/imgui_texture.hpp"
#include "editor/overlay_drawer.hpp"
#include "editor/raster_renderer.hpp"
#include "editor/render_structures.hpp"
#include "imgui/imgui.h"

class EditorRenderer {
public:
    static constexpr const char* kDisplayPassName = "DisplayPass";
    static constexpr const char* kDebugPassName = "DebugPass";
    static constexpr const char* kUiPassName = "UiPass";
    static constexpr float kOverlayLineWidth = 2.0f;

    void init();
    void addPasses(VkSmol::RenderGraph& graph);
    void registerImGuiTextures();
    void resize(VkExtent2D renderExtent, VkExtent2D viewportExtent);
    void prepare();

    ImTextureID getDisplayTexId() const { return displayTex; }
    ImTextureID getDebugTexId() const { return debugTex; }
    ImTextureID getOutputTexId() const { return outputTex; }
    OverlayDrawer& getOverlayDrawer() { return overlayDrawer; }
    ViewportShading getShading() const { return shading; }

private:
    DebugUBO debugUBO{};
    DisplayUBO displayUBO{};
    OverlayDrawer overlayDrawer;
    RasterRenderer rasterRenderer;
    ViewportShading shading = ViewportShading::PathTraced;

    VkSmol::ImageHandle displayImageHandle;
    VkSmol::ImageHandle rasterImageHandle;
    VkSmol::ImageHandle depthImageHandle;
    VkSmol::ImageHandle debugImageHandle;

    VkSmol::BufferHandle debugUBOHandle;
    VkSmol::BufferHandle displayUBOHandle;

    VkExtent2D renderExtent = {};
    VkExtent2D viewportExtent = {};

    ui::ImGuiTexture displayTex;
    ui::ImGuiTexture debugTex;
    ui::ImGuiTexture outputTex;
};
