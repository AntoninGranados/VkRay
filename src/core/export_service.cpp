#include "export_service.hpp"

#include <array>
#include <format>

#include <glm/glm.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"

#include "utils/log.hpp"
#include "core/structures.hpp"

namespace {

class ExrChannelBuffer {
public:
    explicit ExrChannelBuffer(int channelCount) {
        InitEXRHeader(&header);
        InitEXRImage(&image);
        header.num_channels = channelCount;
        header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * channelCount);
        header.pixel_types = (int*)malloc(sizeof(int) * channelCount);
        header.requested_pixel_types = (int*)malloc(sizeof(int) * channelCount);
        for (int i = 0; i < channelCount; i++) {
            header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
        }
    }
    ~ExrChannelBuffer() {
        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
    }
    ExrChannelBuffer(const ExrChannelBuffer&) = delete;
    ExrChannelBuffer& operator=(const ExrChannelBuffer&) = delete;

    void setChannelName(int index, const std::string& name) {
        strncpy(header.channels[index].name, name.c_str(), 255);
        header.channels[index].name[std::min<size_t>(name.size(), 255)] = '\0';
    }

    EXRHeader header;
    EXRImage image;
};

template <int N, typename Accessor>
void pushChannel(std::vector<std::pair<std::string, std::vector<float>>>& channels,
                  size_t pixelCount, const std::string& baseName,
                  std::initializer_list<const char*> suffixes, Accessor accessor) {
    std::array<std::vector<float>, N> comps;
    for (auto& c : comps) c.resize(pixelCount);
    for (size_t i = 0; i < pixelCount; i++) {
        if constexpr (N == 1) {
            comps[0][i] = accessor(i);
        } else {
            auto v = accessor(i);
            for (int k = 0; k < N; k++) comps[k][i] = v[k];
        }
    }
    for (int k = 0; k < N; k++)
        channels.emplace_back(baseName + "." + suffixes.begin()[k], std::move(comps[k]));
}

} // namespace

void ExportService::init(VkSmol& engine, uint32_t _width, uint32_t _height, BufferHandle pixelInfoHandle) {
    width  = _width;
    height = _height;
    buffer = engine.createReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
    pixelInfoBufferHandle  = pixelInfoHandle;
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
    buffer = engine.createReadbackBuffer(static_cast<size_t>(width) * height * 4 * sizeof(float));
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

    ExrChannelBuffer exr(4);
    exr.image.images       = (unsigned char**)image_ptr;
    exr.image.width        = width;
    exr.image.height       = height;
    exr.image.num_channels = 4;
    exr.setChannelName(0, "A");
    exr.setChannelName(1, "B");
    exr.setChannelName(2, "G");
    exr.setChannelName(3, "R");

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&exr.image, &exr.header, path.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        FreeEXRErrorMessage(err);
        Log::error("ExportService", "Failed to write EXR");
    } else {
        Log::success("ExportService", std::format("Saved screenshot to {}", path.string()));
    }
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

    if (f.positionW)
        pushChannel<3>(channels, pixelCount, "position_w", {"X", "Y", "Z"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.positionW : glm::vec3(0.0f); });

    if (f.position)
        pushChannel<3>(channels, pixelCount, "position", {"X", "Y", "Z"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.position : glm::vec3(0.0f); });

    if (f.normalW)
        pushChannel<3>(channels, pixelCount, "normal_w", {"X", "Y", "Z"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.normalW : glm::vec3(0.0f); });

    if (f.normal)
        pushChannel<2>(channels, pixelCount, "normal", {"X", "Y"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.normal : glm::vec2(0.0f); });

    if (f.albedo)
        pushChannel<3>(channels, pixelCount, "albedo", {"R", "G", "B"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.albedo : glm::vec3(0.0f); });

    if (f.roughness)
        pushChannel<1>(channels, pixelCount, "roughness", {"V"},
            [&](size_t i) { return pixels[i].aov.hitValid ? pixels[i].aov.roughness : 0.0f; });

    if (f.matType)
        pushChannel<1>(channels, pixelCount, "mat_type", {"V"},
            [&](size_t i) { return pixels[i].aov.hitValid ? static_cast<float>(pixels[i].aov.matType) : -1.0f; });

    if (f.skyMask)
        pushChannel<1>(channels, pixelCount, "sky_mask", {"V"},
            [&](size_t i) { int c = pixels[i].count; return c > 0 ? static_cast<float>(pixels[i].aov.skyMask) / static_cast<float>(c) : 0.0f; });

    if (channels.empty()) return;

    std::sort(channels.begin(), channels.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    int nch = static_cast<int>(channels.size());

    std::vector<float*> ptrs(nch);
    for (int i = 0; i < nch; i++) ptrs[i] = channels[i].second.data();

    ExrChannelBuffer exr(nch);
    exr.image.images       = (unsigned char**)ptrs.data();
    exr.image.width        = width;
    exr.image.height       = height;
    exr.image.num_channels = nch;
    for (int i = 0; i < nch; i++) exr.setChannelName(i, channels[i].first);

    auto stem = basePath.stem().string();
    auto aovPath = (basePath.parent_path() / (stem + "_aovs.exr")).string();

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&exr.image, &exr.header, aovPath.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        FreeEXRErrorMessage(err);
        Log::error("ExportService", "Failed to write AOV EXR");
    } else {
        Log::success("ExportService", std::format("Saved AOVs to {}", aovPath));
    }
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
