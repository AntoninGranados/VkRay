#pragma once

#include <filesystem>

#include "VkSmol/engine.hpp"
#include "app/app_context.hpp"

#define OUTPUT_DIR "outputs"
#define RENDER_OUTPUT_PATH OUTPUT_DIR "/screenshots"
#define ANIMATION_FRAMES_DIR OUTPUT_DIR "/frames"
#define ANIMATION_VIDEO_PATH OUTPUT_DIR "/out.mp4"


class ExportService {
public:
    void init(VkSmol& engine, const uint32_t& width, const uint32_t& height);
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
    void convertFramesToVideo();

    Buffer buffer;

    bool renderRequested   = false;
    bool renderPendingSave = false;
    std::filesystem::path pendingOutputPath;
    uint32_t width, height;
};
