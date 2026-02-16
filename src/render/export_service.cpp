#include "export_service.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include "app/notification_handler.hpp"
#include "app/animation_handler.hpp"
#include "../ui_handler.hpp"

void ExportService::init(VkSmol& engine, const uint32_t& _width, const uint32_t& _height) {
    width = _width;
    height = _height;

    // Assumes that the image is using the format `VK_FORMAT_R32G32B32A32_SFLOAT`
    buffer = engine.initReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
}

void ExportService::destroy(VkSmol& engine) {
    engine.destroyBuffer(buffer);
}

void ExportService::handleCopy(AppContext& ctx, CommandBuffer& commandBuffer, Image& image) {
    if (!renderRequested) return;
    renderRequested   = false;
    renderPendingSave = true;

    copyImageToBuffer(ctx, commandBuffer, image);
}

void ExportService::handleSave(AppContext& ctx) {
    if (!renderPendingSave) return;
    renderPendingSave = false;

    ctx.engine->waitIdle();
    std::string path = buildRenderOutputPath();
    
    bool toVideo = false;
    if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
        path = buildAnimationoFramePath(ctx.animation->getFrame());

        ctx.renderState->samplesPerSecEMA = 0.0;
        ctx.renderState->samplesPerSecInitialized = false;
        ctx.renderState->samplesPerSecAccumTime = 0.0;
        ctx.renderState->samplesPerSecAccumSamples = 0.0;

        ctx.animation->stepFixed();
        if (ctx.animation->getFrame() == 0) {
            ctx.renderState->pendingExit = true;
            toVideo = true;
        }
    }

    if (ctx.renderState->pendingExit) {
        ctx.ui->restorToggledState();
        ctx.renderState->renderMode = RenderMode::Preview;
        ctx.renderState->pendingExit = false;
        ctx.renderState->samplesPerSecEMA = 0.0;
        ctx.renderState->samplesPerSecInitialized = false;
        ctx.renderState->samplesPerSecAccumTime = 0.0;
        ctx.renderState->samplesPerSecAccumSamples = 0.0;
    }

    *ctx.restartRender = true;
    saveBufferToFile(ctx, path);
    if (toVideo) convertFramesToVideo();
}


std::string ExportService::buildRenderOutputPath() {
    auto now = std::chrono::system_clock::now();
    auto nowMilli = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = nowMilli.time_since_epoch().count();

    char buff[64];
    std::snprintf(buff, 64, "%s/screenshot_%lld.png", RENDER_OUTPUT_PATH, value);
    return std::string(buff);
}

std::string ExportService::buildAnimationoFramePath(int frame) {
    char buff[64];
    std::snprintf(buff, 64, "%s/frame_%05d.png", ANIMATION_FRAMES_DIR, frame);
    return std::string(buff);
}


void ExportService::copyImageToBuffer(AppContext& ctx, CommandBuffer& commandBuffer, Image& image) {
    VkSmol& engine = *ctx.engine;
    
    engine.barrier(
        commandBuffer,
        image.get(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    image.copyToBuffer(commandBuffer, buffer);

    engine.barrier(
        commandBuffer,
        image.get(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
}

void ExportService::saveBufferToFile(AppContext& ctx, std::string path) {
    VkSmol& engine = *ctx.engine;

    size_t floatCount = static_cast<size_t>(width) * height * 4;
    size_t byteCount = floatCount * sizeof(float);
    std::vector<float> floatPixels(floatCount);
    engine.readBuffer(buffer, floatPixels.data(), byteCount);

    std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
    auto toByte = [](float v) -> uint8_t {
        v = std::clamp(v, 0.0f, 1.0f);
        v = std::pow(v, 1.0f / 2.2f);
        return static_cast<uint8_t>(v * 255.0f + 0.5f);
    };
    for (size_t i = 0; i < floatCount; i += 4) {
        pixels[i + 0] = toByte(floatPixels[i + 0]);
        pixels[i + 1] = toByte(floatPixels[i + 1]);
        pixels[i + 2] = toByte(floatPixels[i + 2]);
        pixels[i + 3] = 255;
    }

    if (stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4, pixels.data(), static_cast<int>(width) * 4) != 0) {
        ctx.notifications->pushMessage(NotificationType::Info, "Saved screenshot to " + path);
    } else {
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to write screenshot");
    }
}

void ExportService::convertFramesToVideo() {
    std::string path = std::string(ANIMATION_FRAMES_DIR) + "/frame_%05d.png";

    char cmd[128];
    std::snprintf(cmd, 128, "ffmpeg -framerate 24 -i %s -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p %s", path.c_str(), ANIMATION_VIDEO_PATH);
    std::system(cmd);
}
