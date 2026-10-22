#pragma once

#include <filesystem>

#include "core/render/pathtrace_renderer.hpp"
#include "export_service.hpp"

class CoreRenderer : public PathtraceRenderer {
public:
    void init();

    void saveCapture(const std::filesystem::path& path);

    void bindParameters();
    VkSmol::ImageHandle getLensImageHandle() const { return lensImageHandle; }
    VkSmol::BufferHandle getBlueNoiseBufferHandle() const { return blueNoiseBufferHandle; }

protected:
    void onResize(uint32_t width, uint32_t height) override;

private:
    ExportService exportService;
    VkSmol::ImageHandle lensImageHandle;
    VkSmol::BufferHandle blueNoiseBufferHandle;
    AOVFlags aovFlags = {};
};
