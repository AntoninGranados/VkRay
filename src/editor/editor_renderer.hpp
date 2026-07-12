#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(RenderGraphBuilder& builder, CoreResources& coreResources);

    void render(const FrameContext& frameContext);
    void setDebugView(int v) { displayUBO.debugView = v; }

private:
    struct DisplayUBO {
        int debugView            = 0;
        int previewBorderEnabled = 0;
    };

    DisplayUBO displayUBO{};

    ImageHandle swapchainImageHandle;
    BufferHandle vertexBufferHandle, indexBufferHandle;
    BufferHandle displayUBOHandle;

    PassHandle displayPassHandle, uiPassHandle, presentPassHandle;
    GraphicsPipelineHandle displayPipelineHandle;

    SubmissionGroupHandle editorGroupHandle = {};
    SubmissionGroupHandle uiGroupHandle = {};

    void displayPass(const FrameContext& frameContext);
    void uiPass();
};
