#pragma once

#include <filesystem>

#include "VkSmol/engine.hpp"
#include "app/app_context.hpp"

static constexpr const char* OUTPUT_DIR           = "outputs";
static constexpr const char* RENDER_OUTPUT_PATH   = "outputs/screenshots";
static constexpr const char* ANIMATION_FRAMES_DIR = "outputs/frames";
static constexpr const char* ANIMATION_VIDEO_PATH = "outputs/out.mp4";


class ExportService {
public:
    void init(VkSmol& engine, const uint32_t& width, const uint32_t& height, BufferHandle pixelInfoHandle);
    void destroy(VkSmol& engine);
    void requestSave() { renderRequested = true; }
    bool promptOutputPath();

    void handleCopy(AppContext& ctx, CommandBuffer& commandBuffer, Image& image);
    void handleSave(AppContext& ctx);
    void captureImage(CommandBuffer& commandBuffer, Image& image);
    void saveCapture(AppContext& ctx, const std::filesystem::path& path);

private:
    std::filesystem::path buildAnimationFramePath(int frame);

    void copyImageToBuffer(CommandBuffer& commandBuffer, Image& image);
    void saveBufferToPNG(AppContext& ctx, const std::filesystem::path& path);
    void saveBufferToEXR(AppContext& ctx, const std::filesystem::path& path);
    void saveAOVs(AppContext& ctx, const std::filesystem::path& basePath);
    void convertFramesToVideo();

    Buffer buffer;
    Buffer pixelInfoReadbackBuffer;
    BufferHandle pixelInfoBufferHandle;

    bool renderRequested   = false;
    bool renderPendingSave = false;
    std::filesystem::path pendingOutputPath;
    uint32_t width, height;
};
