#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"
#include "editor/structures.hpp"
#include "imgui/imgui.h"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(RenderGraphBuilder& builder, RenderResources& renderResources);
    void registerImGuiTextures();
    void resize(VkExtent2D renderExtent, VkExtent2D viewportExtent);
    void render(const FrameContext& frameContext);

    ImTextureID getDisplayTexId() const { return displayTexId; }
    ImTextureID getDebugTexId()   const { return debugTexId; }
    ImTextureID getOutputTexId()  const { return outputTexId; }

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

    ImTextureID displayTexId = 0;
    ImTextureID debugTexId   = 0;
    ImTextureID outputTexId  = 0;
};
