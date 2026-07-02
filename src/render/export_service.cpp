#include "export_service.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

#include <nfd.hpp>

#include "app/notification_handler.hpp"
#include "app/parameter_handler.hpp"
#include "app/animation_handler.hpp"
#include "editor/editor_ui.hpp"

void ExportService::init(VkSmol& engine, const uint32_t& _width, const uint32_t& _height, BufferHandle pixelInfoHandle) {
    width  = _width;
    height = _height;
    buffer                  = engine.createReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
    pixelInfoBufferHandle   = pixelInfoHandle;
    pixelInfoReadbackBuffer = engine.createReadbackBuffer(engine.getBuffer(pixelInfoHandle).getSize());
}

void ExportService::destroy(VkSmol& engine) {
    engine.destroyBuffer(buffer);
    engine.destroyBuffer(pixelInfoReadbackBuffer);
}

void ExportService::handleCopy(AppContext& ctx, CommandBuffer& commandBuffer, Image& image) {
    if (!renderRequested) return;
    renderRequested   = false;
    renderPendingSave = true;

    copyImageToBuffer(commandBuffer, image);
}

bool ExportService::promptOutputPath() {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    nfdfilteritem_t filters[2] = {
        { "PNG Image",     "png" },
        { "OpenEXR Image", "exr" }
    };
    if (NFD::SaveDialog(outPath, filters, 2, RENDER_OUTPUT_PATH, "render") != NFD_OKAY)
        return false;
    pendingOutputPath = outPath.get();
    if (!pendingOutputPath.has_extension())
        pendingOutputPath.replace_extension(".png");
    return true;
}

void ExportService::handleSave(AppContext& ctx) {
    if (!renderPendingSave) return;
    renderPendingSave = false;

    ctx.engine->waitIdle();

    bool toVideo = false;
    bool doSave  = false;
    std::filesystem::path path;

    if (ctx.renderState->renderMode == RenderMode::RenderAnimation) {
        path   = buildAnimationFramePath(ctx.animation->getFrame());
        doSave = true;

        ctx.renderState->samplesPerSecEMA          = 0.0;
        ctx.renderState->samplesPerSecInitialized  = false;
        ctx.renderState->samplesPerSecAccumTime    = 0.0;
        ctx.renderState->samplesPerSecAccumSamples = 0.0;

        ctx.animation->stepFixed();
        if (ctx.animation->getFrame() == 0) {
            ctx.renderState->pendingExit = true;
            toVideo = true;
        }
    } else if (!pendingOutputPath.empty()) {
        path   = pendingOutputPath;
        doSave = true;
        pendingOutputPath.clear();
    }

    if (ctx.renderState->pendingExit) {
        ctx.ui->restoreToggledState();
        ctx.renderState->renderMode                = RenderMode::Preview;
        ctx.renderState->pendingExit               = false;
        ctx.renderState->samplesPerSecEMA          = 0.0;
        ctx.renderState->samplesPerSecInitialized  = false;
        ctx.renderState->samplesPerSecAccumTime    = 0.0;
        ctx.renderState->samplesPerSecAccumSamples = 0.0;
    }

    *ctx.restartRender = true;

    if (doSave) {
        if (path.extension() == ".exr") {
            saveBufferToEXR(ctx, path);
        } else {
            saveBufferToPNG(ctx, path);
        }
        saveAOVs(ctx, path);
    }

    if (toVideo) convertFramesToVideo();
}

void ExportService::captureImage(CommandBuffer& commandBuffer, Image& image) {
    copyImageToBuffer(commandBuffer, image);
}

void ExportService::saveCapture(AppContext& ctx, const std::filesystem::path& path) {
    ctx.engine->waitIdle();
    if (path.extension() == ".exr") {
        saveBufferToEXR(ctx, path);
    } else {
        saveBufferToPNG(ctx, path);
    }
    saveAOVs(ctx, path);
}


std::filesystem::path ExportService::buildAnimationFramePath(int frame) {
    char buff[64];
    std::snprintf(buff, sizeof(buff), "frame_%05d.png", frame);

    std::filesystem::path path = ANIMATION_FRAMES_DIR;
    path /= buff;
    return path;
}


void ExportService::copyImageToBuffer(CommandBuffer& commandBuffer, Image& image) {
    image.copyToBuffer(commandBuffer, buffer);
}

void ExportService::saveBufferToPNG(AppContext& ctx, const std::filesystem::path& path) {
    VkSmol& engine = *ctx.engine;

    size_t floatCount = static_cast<size_t>(width) * height * 4;
    size_t byteCount  = floatCount * sizeof(float);
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
        ctx.notifications->pushMessage(NotificationType::Info, "Saved screenshot to " + path.string());
    } else {
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to write screenshot");
    }
}

void ExportService::saveBufferToEXR(AppContext& ctx, const std::filesystem::path& path) {
    VkSmol& engine = *ctx.engine;

    size_t floatCount = static_cast<size_t>(width) * height * 4;
    size_t byteCount  = floatCount * sizeof(float);
    std::vector<float> floatPixels(floatCount);
    engine.readBuffer(buffer, floatPixels.data(), byteCount);

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 4;

    std::vector<float> images[4];
    images[0].resize(width * height);
    images[1].resize(width * height);
    images[2].resize(width * height);
    images[3].resize(width * height);

    for (int i = 0; i < static_cast<int>(width * height); i++) {
        images[0][i] = floatPixels[4 * i + 0];
        images[1][i] = floatPixels[4 * i + 1];
        images[2][i] = floatPixels[4 * i + 2];
        images[3][i] = 1.0f;
    }

    float* image_ptr[4];
    image_ptr[0] = &(images[3].at(0)); // A
    image_ptr[1] = &(images[2].at(0)); // B
    image_ptr[2] = &(images[1].at(0)); // G
    image_ptr[3] = &(images[0].at(0)); // R

    image.images = (unsigned char**)image_ptr;
    image.width  = width;
    image.height = height;

    header.num_channels = 4;
    header.channels     = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    strncpy(header.channels[0].name, "A", 255); header.channels[0].name[strlen("A")] = '\0';
    strncpy(header.channels[1].name, "B", 255); header.channels[1].name[strlen("B")] = '\0';
    strncpy(header.channels[2].name, "G", 255); header.channels[2].name[strlen("G")] = '\0';
    strncpy(header.channels[3].name, "R", 255); header.channels[3].name[strlen("R")] = '\0';

    header.pixel_types           = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i]           = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, path.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        FreeEXRErrorMessage(err);
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to write EXR");
    } else {
        ctx.notifications->pushMessage(NotificationType::Info, "Saved screenshot to " + path.string());
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void ExportService::saveAOVs(AppContext& ctx, const std::filesystem::path& basePath) {
    auto& p = *ctx.parameters;
    bool anyEnabled = p.getBool("pathtracer/aov/normal")        || p.getBool("pathtracer/aov/normal_opaque")
                   || p.getBool("pathtracer/aov/albedo")        || p.getBool("pathtracer/aov/albedo_opaque")
                   || p.getBool("pathtracer/aov/depth")         || p.getBool("pathtracer/aov/depth_opaque")
                   || p.getBool("pathtracer/aov/sky_mask")       || p.getBool("pathtracer/aov/sky_mask_opaque");
    if (!anyEnabled) return;

    VkSmol& engine = *ctx.engine;
    engine.copyBuffer(engine.getBuffer(pixelInfoBufferHandle), pixelInfoReadbackBuffer);

    size_t pixelCount = static_cast<size_t>(width) * height;
    std::vector<PixelInfo> pixels(pixelCount);
    engine.readBuffer(pixelInfoReadbackBuffer, pixels.data(), pixelCount * sizeof(PixelInfo));

    std::vector<std::pair<std::string, std::vector<float>>> channels;

    auto push = [&](const std::string& name, std::vector<float> data) {
        channels.emplace_back(name, std::move(data));
    };

    if (p.getBool("pathtracer/aov/albedo")) {
        std::vector<float> r(pixelCount), g(pixelCount), b(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            r[i] = pixels[i].aov.hitValid ? pixels[i].aov.albedo.x : 0.0f;
            g[i] = pixels[i].aov.hitValid ? pixels[i].aov.albedo.y : 0.0f;
            b[i] = pixels[i].aov.hitValid ? pixels[i].aov.albedo.z : 0.0f;
        }
        push("albedo.R", std::move(r));
        push("albedo.G", std::move(g));
        push("albedo.B", std::move(b));
    }

    if (p.getBool("pathtracer/aov/albedo_opaque")) {
        std::vector<float> r(pixelCount), g(pixelCount), b(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            r[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.albedoOpaque.x : 0.0f;
            g[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.albedoOpaque.y : 0.0f;
            b[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.albedoOpaque.z : 0.0f;
        }
        push("albedoOpaque.R", std::move(r));
        push("albedoOpaque.G", std::move(g));
        push("albedoOpaque.B", std::move(b));
    }

    if (p.getBool("pathtracer/aov/normal")) {
        std::vector<float> x(pixelCount), y(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.camNormal.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.camNormal.y : 0.0f;
        }
        push("normal.X", std::move(x));
        push("normal.Y", std::move(y));
    }

    if (p.getBool("pathtracer/aov/normal_opaque")) {
        std::vector<float> x(pixelCount), y(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.camNormalOpaque.x : 0.0f;
            y[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.camNormalOpaque.y : 0.0f;
        }
        push("normalOpaque.X", std::move(x));
        push("normalOpaque.Y", std::move(y));
    }

    if (p.getBool("pathtracer/aov/depth")) {
        std::vector<float> d(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            d[i] = pixels[i].aov.hitValid ? pixels[i].aov.depth : -1.0f;
        push("depth.Z", std::move(d));
    }

    if (p.getBool("pathtracer/aov/depth_opaque")) {
        std::vector<float> d(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            d[i] = pixels[i].aov.opaqueHitValid ? pixels[i].aov.depthOpaque : -1.0f;
        push("depthOpaque.Z", std::move(d));
    }

    if (p.getBool("pathtracer/aov/sky_mask")) {
        std::vector<float> m(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            int c = pixels[i].count;
            m[i] = c > 0 ? static_cast<float>(pixels[i].aov.skyMask) / static_cast<float>(c) : 0.0f;
        }
        push("skyMask.V", std::move(m));
    }

    if (p.getBool("pathtracer/aov/sky_mask_opaque")) {
        std::vector<float> m(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            int c = pixels[i].count;
            m[i] = c > 0 ? static_cast<float>(pixels[i].aov.skyMaskOpaque) / static_cast<float>(c) : 0.0f;
        }
        push("skyMaskOpaque.V", std::move(m));
    }

    if (channels.empty()) return;

    std::sort(channels.begin(), channels.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    int nch = static_cast<int>(channels.size());

    EXRHeader header; InitEXRHeader(&header);
    EXRImage  image;  InitEXRImage(&image);

    std::vector<float*> ptrs(nch);
    for (int i = 0; i < nch; i++) ptrs[i] = channels[i].second.data();
    image.images = (unsigned char**)ptrs.data();
    image.width  = width;
    image.height = height;
    image.num_channels = nch;

    header.num_channels = nch;
    header.channels     = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * nch);
    header.pixel_types           = (int*)malloc(sizeof(int) * nch);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * nch);
    for (int i = 0; i < nch; i++) {
        strncpy(header.channels[i].name, channels[i].first.c_str(), 255);
        header.pixel_types[i]           = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    auto stem = basePath.stem().string();
    auto aovPath = (basePath.parent_path() / (stem + "_aovs.exr")).string();

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, aovPath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        FreeEXRErrorMessage(err);
        ctx.notifications->pushMessage(NotificationType::Error, "Failed to write AOV EXR");
    } else {
        ctx.notifications->pushMessage(NotificationType::Info, "Saved AOVs to " + aovPath);
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void ExportService::convertFramesToVideo() {
    std::filesystem::path path = ANIMATION_FRAMES_DIR;
    path /= "frame_%05d.png";

    char cmd[128];
    std::snprintf(cmd, 128, "ffmpeg -framerate 24 -i %s -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p %s", path.c_str(), ANIMATION_VIDEO_PATH);
    std::system(cmd);
}
