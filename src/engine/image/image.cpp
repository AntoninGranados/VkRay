#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imageBarrier.hpp"
#include "../logicalDevice.hpp"
#include "../physicalDevice.hpp"
#include "../queue.hpp"
#include "../command/commandBuffer.hpp"
#include "../command/commandPool.hpp"
#include "../memory/buffer.hpp"
#include "../utils.hpp"

void Image::init(
    LogicalDevice device,
    uint32_t width_, uint32_t height_,
    VkFormat format_, VkImageUsageFlags usage
) {
    width = width_;
    height = height_;
    format = format_;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    imageInfo.format = format;
    imageInfo.usage = usage;

    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCheck(vkCreateImage(device.get(), &imageInfo, nullptr, &image), "Failed to create image");
}

void Image::initFromPath(
    LogicalDevice device, PhysicalDevice physicalDevice,
    CommandPool transferCommandPool, Queue transferQueue,
    std::string path
) {
    int width, height, channels;
    stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize imageSize = width * height * 4;

    if (!data)
        throw std::runtime_error("failed to load texture image!");

    Buffer stagingBuffer;
    stagingBuffer.init(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize);
    stagingBuffer.alloc(device, physicalDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.fill(device, data);

    stbi_image_free(data);

    init(
        device,
        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
    alloc(device, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ImageBarrier barrier;
    CommandBuffer commandBuffer = CommandBuffer::beginSingleTimeCommands(device, transferCommandPool);

    barrier.call(
        commandBuffer,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );
    
    fillFromBuffer(device, commandBuffer, stagingBuffer);
    
    //! hardcoded destinations -> should be parameters (layout + access + stage)
    barrier.call(
        commandBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );

    CommandBuffer::endSingleTimeCommands(device, transferCommandPool, transferQueue, commandBuffer);

    stagingBuffer.free(device);
    stagingBuffer.destroy(device);
}

void Image::fillFromBuffer(
    LogicalDevice device, CommandBuffer commandBuffer,
    Buffer buffer
) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        commandBuffer.get(),
        buffer.get(),
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void Image::alloc(LogicalDevice device, PhysicalDevice physicalDevice, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.get(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Buffer::findMemoryType(
        physicalDevice,
        memRequirements.memoryTypeBits,
        properties
    );

    vkCheck(vkAllocateMemory(device.get(), &allocInfo, nullptr, &imageMemory), "Failed to allocate image memory");
    vkCheck(vkBindImageMemory(device.get(), image, imageMemory, 0), "Failed to bind image memory");
}

void Image::free(LogicalDevice device) {
    vkFreeMemory(device.get(), imageMemory, nullptr);
}

void Image::destroy(LogicalDevice device) {
    vkDestroyImage(device.get(), image, nullptr);
}
