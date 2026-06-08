#pragma once

#include <string>

#include "engine/engine.hpp"
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

    void handleCopy(AppContext& ctx, CommandBuffer& commandBuffer, Image& image);
    void handleSave(AppContext& ctx);
    
private:
    std::string buildRenderOutputPath();
    std::string buildAnimationoFramePath(int frame);
    
    void copyImageToBuffer(CommandBuffer& commandBuffer, Image& image);
    void saveBufferToFile(AppContext& ctx, std::string path);
    void convertFramesToVideo();

    Buffer buffer;

    bool renderRequested = false;
    bool renderPendingSave = false;
    uint32_t width, height;
};
