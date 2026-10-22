#pragma once

#include <filesystem>
#include <vector>

#include "VkSmol/engine.hpp"

#include "core/render_structures.hpp"

class ExportService {
public:
    void init(uint32_t width, uint32_t height, VkSmol::BufferHandle pixelInfoHandle);
    void resize(uint32_t width, uint32_t height);

    void save(VkSmol::Engine& engine, VkSmol::ImageHandle image, const std::filesystem::path& path,
              const AOVFlags& aovFlags = {});

    static void convertFramesToVideo(const std::filesystem::path& output, const std::filesystem::path& framePath,
                                     int fps);
    static std::filesystem::path buildAnimationFramePath(int frame, const std::filesystem::path& dir);

private:
    void saveBufferToPNG(const std::vector<float>& floatPixels, const std::filesystem::path& path);
    void saveBufferToEXR(const std::vector<float>& floatPixels, const std::filesystem::path& path);
    void saveAOVs(VkSmol::Engine& engine, const std::filesystem::path& basePath, const AOVFlags& aovFlags);

    VkSmol::BufferHandle pixelInfoBufferHandle;
    uint32_t width = 0, height = 0;
};
