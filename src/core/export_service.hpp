#pragma once

#include <filesystem>

#include "VkSmol/engine.hpp"

#include "core/render_structures.hpp"

class ExportService {
public:
    void init(VkSmol& engine, uint32_t width, uint32_t height, BufferHandle pixelInfoHandle);
    void destroy(VkSmol& engine);
    void resize(VkSmol& engine, uint32_t width, uint32_t height);

    void save(VkSmol& engine, Image& image, const std::filesystem::path& path, const AOVFlags& aovFlags = {});

    static void convertFramesToVideo(const std::filesystem::path& output, const std::filesystem::path& framePath);
    static std::filesystem::path buildAnimationFramePath(int frame, const std::filesystem::path& dir);

private:
    void saveBufferToPNG(VkSmol& engine, const std::filesystem::path& path);
    void saveBufferToEXR(VkSmol& engine, const std::filesystem::path& path);
    void saveAOVs(VkSmol& engine, const std::filesystem::path& basePath, const AOVFlags& aovFlags);

    Buffer       buffer;
    Buffer       pixelInfoReadbackBuffer;
    BufferHandle pixelInfoBufferHandle;
    uint32_t     width = 0, height = 0;
};
