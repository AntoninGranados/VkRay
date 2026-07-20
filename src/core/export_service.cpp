#include "export_service.hpp"

#include <format>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

#include "utils/log.hpp"
#include "core/structures.hpp"

void ExportService::init(VkSmol& engine, uint32_t _width, uint32_t _height, BufferHandle pixelInfoHandle) {
    width  = _width;
    height = _height;
    buffer                  = engine.createReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
    pixelInfoBufferHandle   = pixelInfoHandle;
    pixelInfoReadbackBuffer = engine.createReadbackBuffer(static_cast<size_t>(width) * height * sizeof(PixelInfo));
}

void ExportService::destroy(VkSmol& engine) {
    engine.destroyBuffer(buffer);
    engine.destroyBuffer(pixelInfoReadbackBuffer);
}

void ExportService::resize(VkSmol& engine, uint32_t _width, uint32_t _height) {
    engine.destroyBuffer(buffer);
    engine.destroyBuffer(pixelInfoReadbackBuffer);
    width  = _width;
    height = _height;
    buffer                  = engine.createReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
    pixelInfoReadbackBuffer = engine.createReadbackBuffer(static_cast<size_t>(width) * height * sizeof(PixelInfo));
}

void ExportService::save(VkSmol& engine, Image& image, const std::filesystem::path& path, const AOVFlags& aovFlags) {
    engine.waitIdle();
    engine.copyImageToBuffer(image, buffer);
    if (path.extension() == ".exr") {
        saveBufferToEXR(engine, path);
    } else {
        saveBufferToPNG(engine, path);
    }
    saveAOVs(engine, path, aovFlags);
}

std::filesystem::path ExportService::buildAnimationFramePath(int frame, const std::filesystem::path& dir) {
    return dir / std::format("frame_{:05d}.png", frame);
}

void ExportService::saveBufferToPNG(VkSmol& engine, const std::filesystem::path& path) {
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

void ExportService::saveBufferToEXR(VkSmol& engine, const std::filesystem::path& path) {
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

void ExportService::saveAOVs(VkSmol& engine, const std::filesystem::path& basePath, const AOVFlags& f) {
    bool anyEnabled = f.positionW || f.position || f.normalW || f.normal
                   || f.albedo   || f.roughness || f.matType || f.skyMask;
    if (!anyEnabled) return;

    engine.copyBuffer(engine.getBuffer(pixelInfoBufferHandle), pixelInfoReadbackBuffer);

    size_t pixelCount = static_cast<size_t>(width) * height;
    std::vector<PixelInfo> pixels(pixelCount);
    engine.readBuffer(pixelInfoReadbackBuffer, pixels.data(), pixelCount * sizeof(PixelInfo));

    std::vector<std::pair<std::string, std::vector<float>>> channels;

    auto push = [&](const std::string& name, std::vector<float> data) {
        channels.emplace_back(name, std::move(data));
    };

    if (f.positionW) {
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

    if (f.position) {
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

    if (f.normalW) {
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

    if (f.normal) {
        std::vector<float> x(pixelCount), y(pixelCount);
        for (size_t i = 0; i < pixelCount; i++) {
            x[i] = pixels[i].aov.hitValid ? pixels[i].aov.normal.x : 0.0f;
            y[i] = pixels[i].aov.hitValid ? pixels[i].aov.normal.y : 0.0f;
        }
        push("normal.X", std::move(x));
        push("normal.Y", std::move(y));
    }

    if (f.albedo) {
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

    if (f.roughness) {
        std::vector<float> v(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            v[i] = pixels[i].aov.hitValid ? pixels[i].aov.roughness : 0.0f;
        push("roughness.V", std::move(v));
    }

    if (f.matType) {
        std::vector<float> v(pixelCount);
        for (size_t i = 0; i < pixelCount; i++)
            v[i] = pixels[i].aov.hitValid ? static_cast<float>(pixels[i].aov.matType) : -1.0f;
        push("mat_type.V", std::move(v));
    }

    if (f.skyMask) {
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
    EXRImage  exrImage;  InitEXRImage(&exrImage);

    std::vector<float*> ptrs(nch);
    for (int i = 0; i < nch; i++) ptrs[i] = channels[i].second.data();
    exrImage.images = (unsigned char**)ptrs.data();
    exrImage.width  = width;
    exrImage.height = height;
    exrImage.num_channels = nch;

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
    int ret = SaveEXRImageToFile(&exrImage, &header, aovPath.c_str(), &err);
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

void ExportService::convertFramesToVideo(const std::filesystem::path& path, const std::filesystem::path& framePath) {
    std::filesystem::path framesPath = framePath / "frame_%05d.png";

    std::string cmd = std::format(
        "ffmpeg -framerate 24 -i {} -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p {}", framesPath.c_str(), path.c_str()
    );
    int ret = std::system(cmd.c_str());
    if (ret == 0)
        Log::success("ExportService", std::format("Video saved to {}", path.c_str()));
    else
        Log::error("ExportService", std::format("ffmpeg exited with code {}", ret));
}
