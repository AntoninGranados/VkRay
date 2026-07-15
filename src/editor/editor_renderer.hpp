#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"
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

private:
    struct DisplayUBO {
        int previewBorderEnabled = 0;
    };

    struct DebugUBO {
        int debugView = 0;
    };

    DisplayUBO displayUBO{};
    DebugUBO   debugUBO{};

    ImageHandle swapchainImageHandle;
    ImageHandle displayImageHandle;
    ImageHandle debugImageHandle;

    BufferHandle displayUBOHandle;
    BufferHandle debugUBOHandle;

    PassHandle displayPassHandle, debugPassHandle, uiPassHandle, presentPassHandle;

    ComputePipelineHandle displayPipelineHandle;
    ComputePipelineHandle debugPipelineHandle;

    SubmissionGroupHandle editorGroupHandle = {};
    SubmissionGroupHandle uiGroupHandle     = {};

    VkExtent2D renderExtent = {};

    ImTextureID displayTexId = 0;
    ImTextureID debugTexId   = 0;

    void editorPass(const FrameContext& frameContext);
    void uiPass();
};
