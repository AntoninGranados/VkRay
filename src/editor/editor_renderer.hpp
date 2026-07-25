#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"
#include "editor/structures.hpp"
#include "imgui/imgui.h"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(RenderGraphBuilder& builder, CoreResources& coreResources);
    void registerImGuiTextures();
    void resize(uint32_t width, uint32_t height);
    void render(const FrameContext& frameContext);

    ImTextureID getDisplayTexId() const { return displayTexId; }
    ImTextureID getDebugTexId()   const { return debugTexId; }
    ImTextureID getOutputTexId()  const { return outputTexId; }

private:
    DebugUBO debugUBO{};

    ImageHandle swapchainImageHandle;
    ImageHandle displayImageHandle;
    ImageHandle debugImageHandle;
    ImageHandle outputImageHandle;

    BufferHandle debugUBOHandle;

    PassHandle displayPassHandle, debugPassHandle, uiPassHandle, presentPassHandle;

    ComputePipelineHandle displayPipelineHandle;
    ComputePipelineHandle debugPipelineHandle;

    SubmissionGroupHandle editorGroupHandle = {};
    SubmissionGroupHandle uiGroupHandle     = {};

    VkExtent2D renderExtent = {};

    ImTextureID displayTexId = 0;
    ImTextureID debugTexId   = 0;
    ImTextureID outputTexId  = 0;

    void editorPass(const FrameContext& frameContext);
    void uiPass();
};
