#pragma once

#include "VkSmol/engine.hpp"
#include "VkSmol/graph/builder_resource.hpp"

#include "app/app_context.hpp"
#include "core/core_renderer.hpp"

struct FrameContext;

class EditorRenderer {
public:
    void initGraph(VkSmol& engine, RenderGraphBuilder& builder, CoreResources& coreResources);

    void render(AppContext& ctx, const FrameContext& frameContext);

private:

ImageHandle swapchainImageHandle;
BufferHandle vertexBufferHandle, indexBufferHandle;
BufferHandle displayUBOHandle;

PassHandle displayPassHandle, uiPassHandle, presentPassHandle;
GraphicsPipelineHandle displayPipelineHandle;

SubmissionGroupHandle editorGroupHandle = {};
SubmissionGroupHandle uiGroupHandle = {};

    void displayPass(AppContext& ctx, const FrameContext& frameContext);
    void uiPass(AppContext& ctx);
};
