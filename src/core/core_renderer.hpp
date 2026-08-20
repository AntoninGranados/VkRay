#pragma once

#include <filesystem>

#include "core/render/pathtrace_renderer.hpp"
#include "export_service.hpp"

class CoreRenderer : public PathtraceRenderer {
public:
    RenderResources initGraph(RenderGraphBuilder& builder);
    void destroy();
    // TODO: make it non blocking (compile/build in the background and replace when finished)
    void buildPipelines();

    void saveCapture(const std::filesystem::path& path);

    void bindParameters();
    ImageHandle getLensImageHandle() const { return lensImageHandle; }

protected:
    void onAfterDispatch(CommandBuffer& commandBuffer) override;
    void onResize(uint32_t width, uint32_t height) override;

private:
    ExportService exportService;
    ImageHandle lensImageHandle;
    AOVFlags aovFlags = {};
    PassHandle exportPassHandle;
};
