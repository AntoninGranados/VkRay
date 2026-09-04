#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"
#include "editor/imgui_texture.hpp"
#include "editor/render_structures.hpp"
#include "imgui/imgui.h"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(RenderGraphBuilder& builder, RenderResources& renderResources);
    void registerImGuiTextures();
    void resize(VkExtent2D renderExtent, VkExtent2D viewportExtent);
    void render(const FrameContext& frameContext);

    ImTextureID getDisplayTexId() const { return displayTex; }
    ImTextureID getDebugTexId()   const { return debugTex; }
    ImTextureID getOutputTexId()  const { return outputTex; }

    TimestampHandle getDisplayTimestamp() const { return displayTimestamp; }
    TimestampHandle getDebugTimestamp()   const { return debugTimestamp; }
    TimestampHandle getUiTimestamp()      const { return uiTimestamp; }

private:
    DebugUBO   debugUBO{};
    DisplayUBO displayUBO{};

    ImageHandle swapchainImageHandle;
    ImageHandle displayImageHandle;
    ImageHandle debugImageHandle;
    ImageHandle outputImageHandle;

    BufferHandle debugUBOHandle;
    BufferHandle displayUBOHandle;

    PassHandle displayPassHandle, debugPassHandle, uiPassHandle, presentPassHandle;

    TimestampHandle displayTimestamp;
    TimestampHandle debugTimestamp;
    TimestampHandle uiTimestamp;

    SubmissionGroupHandle editorGroupHandle = {};
    SubmissionGroupHandle uiGroupHandle     = {};

    VkExtent2D renderExtent   = {};
    VkExtent2D viewportExtent = {};

    ui::ImGuiTexture displayTex;
    ui::ImGuiTexture debugTex;
    ui::ImGuiTexture outputTex;
};
