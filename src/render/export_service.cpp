#include "export_service.hpp"

#include <format>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

#include <nfd.hpp>

#include "app/log.hpp"
#include "app/parameter_handler.hpp"
#include "app/animation_handler.hpp"

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
        if (onRenderComplete) onRenderComplete();
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
    return std::filesystem::path(ANIMATION_FRAMES_DIR) / std::format("frame_{:05d}.png", frame);
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
        Log::success("ExportService", std::format("Saved screenshot to {}", path.string()));
    } else {
        Log::error("ExportService", "Failed to write screenshot");
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
        Log::error("ExportService", "Failed to write EXR");
    } else {
        Log::success("ExportService", std::format("Saved screenshot to {}", path.string()));
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

void ExportService::saveAOVs(AppContext& ctx, const std::filesystem::path& basePath) {
    auto& p = *ctx.parameters;
    bool anyEnabled = p.getBool("pathtracer/aov/position_w")
                   || p.getBool("pathtracer/aov/position")
                   || p.getBool("pathtracer/aov/normal_w")
                   || p.getBool("pathtracer/aov/normal")
                   || p.getBool("pathtracer/aov/albedo")
                   || p.getBool("pathtracer/aov/roughness")
                   || p.getBool("pathtracer/aov/mat_type")
                   || p.getBool("pathtracer/aov/sky_mask");
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

    if (p.getBool("pathtracer/aov/position_w")) {
        std::vector<float> x(pixelCount), y(pixelCount), z(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.positionW.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.positionW.y : 0.0f;
            z[i] = pixels[i].aov.hitValid ? pixels[i].aov.positionW.z : 0.0f;
        }
        push("position_w.X", std::move(x));
        push("position_w.Y", std::move(y));
        push("position_w.Z", std::move(z));
    }

    if (p.getBool("pathtracer/aov/position")) {
        std::vector<float> x(pixelCount), y(pixelCount), z(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.position.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.position.y : 0.0f;
            z[i] = pixels[i].aov.hitValid ? pixels[i].aov.position.z : 0.0f;
        }
        push("position.X", std::move(x));
        push("position.Y", std::move(y));
        push("position.Z", std::move(z));
    }

    if (p.getBool("pathtracer/aov/normal_w")) {
        std::vector<float> x(pixelCount), y(pixelCount), z(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.normalW.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.normalW.y : 0.0f;
            z[i] = pixels[i].aov.hitValid ? pixels[i].aov.normalW.z : 0.0f;
        }
        push("normal_w.X", std::move(x));
        push("normal_w.Y", std::move(y));
        push("normal_w.Z", std::move(z));
    }

    if (p.getBool("pathtracer/aov/normal")) {
        std::vector<float> x(pixelCount), y(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.normal.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.normal.y : 0.0f;
        }
        push("normal.X", std::move(x));
        push("normal.Y", std::move(y));
    }

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

    if (p.getBool("pathtracer/aov/roughness")) {
        std::vector<float> v(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            v[i] = pixels[i].aov.hitValid ? pixels[i].aov.roughness : 0.0f;
        push("roughness.V", std::move(v));
    }

    if (p.getBool("pathtracer/aov/mat_type")) {
        std::vector<float> v(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            v[i] = pixels[i].aov.hitValid ? static_cast<float>(pixels[i].aov.matType) : -1.0f;
        push("mat_type.V", std::move(v));
    }

    if (p.getBool("pathtracer/aov/sky_mask")) {
        std::vector<float> m(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            int c = pixels[i].count;
            m[i] = c > 0 ? static_cast<float>(pixels[i].aov.skyMask) / static_cast<float>(c) : 0.0f;
        }
        push("sky_mask.V", std::move(m));
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
        Log::error("ExportService", "Failed to write AOV EXR");
    } else {
        Log::success("ExportService", std::format("Saved AOVs to {}", aovPath));
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
    int ret = std::system(cmd);
    if (ret == 0)
        Log::success("ExportService", std::format("Video saved to {}", ANIMATION_VIDEO_PATH));
    else
        Log::error("ExportService", std::format("ffmpeg exited with code {}", ret));
}
