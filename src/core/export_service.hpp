#pragma once

#include <filesystem>
#include <vector>

#include "VkSmol/engine.hpp"

class PathtraceRenderer;

class ExportService {
public:
    static void save(const PathtraceRenderer& renderer, const std::filesystem::path& path);

    static void convertFramesToVideo(const std::filesystem::path& output, const std::filesystem::path& framePath,
                                     int fps);
    static std::filesystem::path buildAnimationFramePath(int frame, const std::filesystem::path& dir);

private:
    static void saveBufferToPNG(const std::vector<float>& floatPixels, uint32_t width, uint32_t height,
                                const std::filesystem::path& path);
    static void saveBufferToEXR(const std::vector<float>& floatPixels, uint32_t width, uint32_t height,
                                const std::filesystem::path& path);
    static void saveAOVs(VkSmol::Engine& engine, VkSmol::BufferHandle pixelInfoBufferHandle, uint32_t width,
                         uint32_t height, const std::filesystem::path& basePath);
};
