#pragma once

#include <functional>

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "core/core_renderer.hpp"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(VkSmol& engine, RenderGraphBuilder& builder, CoreResources& coreResources);

    void render(VkSmol& engine, const FrameContext& frameContext, const std::function<void(CommandBuffer&)>& uiDraw = {});
    void setPreviewBorder(bool enabled) { displayUBO.previewBorderEnabled = enabled ? 1 : 0; }
    void setDebugView(int v)            { displayUBO.debugView = v; }

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

    void displayPass(VkSmol& engine, const FrameContext& frameContext);
    void uiPass(VkSmol& engine, const std::function<void(CommandBuffer&)>& uiDraw);
};
