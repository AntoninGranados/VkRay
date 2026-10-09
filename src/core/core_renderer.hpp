#pragma once

#include <filesystem>

#include "core/render/pathtrace_renderer.hpp"
#include "export_service.hpp"

class CoreRenderer : public PathtraceRenderer {
public:
    RenderResources initGraph(RenderGraphBuilder& builder);

    void saveCapture(const std::filesystem::path& path);

    void bindParameters();
    ImageHandle getLensImageHandle() const { return lensImageHandle; }
    BufferHandle getBlueNoiseBufferHandle() const { return blueNoiseBufferHandle; }

protected:
    void onAfterDispatch(CommandBuffer& commandBuffer) override;
    void onResize(uint32_t width, uint32_t height) override;

private:
    ExportService exportService;
    ImageHandle lensImageHandle;
    BufferHandle blueNoiseBufferHandle;
    AOVFlags aovFlags = {};
    PassHandle exportPassHandle;
};
