#include "buffer.hpp"

#include "../physicalDevice.hpp"
#include "../logicalDevice.hpp"
#include "../command/commandPool.hpp"
#include "../command/commandBuffer.hpp"
#include "../queue.hpp"
#include "../utils.hpp"

void Buffer::init(LogicalDevice device,
        VkBufferUsageFlags usage_,
        size_t size_,
        bool persistentMapping_
) {
    usage = usage_;
    size = size_;
    persistentMapping = persistentMapping_;

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;     //! Can only be used in one queue

    vkCheck(vkCreateBuffer(device.get(), &createInfo, nullptr, &buffer), "Failed to create buffer");
}

void Buffer::initFilled(
        LogicalDevice device, PhysicalDevice physicalDevice,
        VkBufferUsageFlags usage,
        CommandPool transferCommandPool, Queue transferQueue,
        size_t size, void *data
) {
    Buffer stagingBuffer;
    stagingBuffer.init(device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
    stagingBuffer.alloc(device, physicalDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.fill(device, data);

    this->init(device, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size);
    this->alloc(device, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    Buffer::copy(stagingBuffer, *this, device, transferCommandPool, transferQueue);

    stagingBuffer.free(device);
    stagingBuffer.destroy(device);
}

void Buffer::alloc(LogicalDevice device, PhysicalDevice physicalDevice, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.get(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Buffer::findMemoryType(
        physicalDevice,
        memRequirements.memoryTypeBits,
        properties
    );

    vkCheck(vkAllocateMemory(device.get(), &allocInfo, nullptr, &bufferMemory), "Failed to allocate buffer memory");

    vkBindBufferMemory(device.get(), buffer, bufferMemory, 0);

    if (persistentMapping)
        vkMapMemory(device.get(), bufferMemory, 0, size, 0, &bufferMapped);
}


void Buffer::free(LogicalDevice device) {
    if (persistentMapping)
    vkUnmapMemory(device.get(), bufferMemory);
    
    vkFreeMemory(device.get(), bufferMemory, nullptr);
}

void Buffer::destroy(LogicalDevice device) {
    vkDestroyBuffer(device.get(), buffer, nullptr);
}

void Buffer::fill(LogicalDevice device, void* data) {
    if (persistentMapping) {
        memcpy(bufferMapped, data, size);
    } else {
        void* mem;
        vkMapMemory(device.get(), bufferMemory, 0, size, 0, &mem);
        memcpy(mem, data, size);
        vkUnmapMemory(device.get(), bufferMemory);
    }
}

void Buffer::copy(Buffer srcBuffer, Buffer dstBuffer, LogicalDevice device, CommandPool transferCommandPool, Queue transferQueue) {
    if (srcBuffer.getSize() != dstBuffer.getSize())
        throw std::runtime_error("Buffers don't have the same size");

    CommandBuffer commandBuffer = CommandBuffer::beginSingleTimeCommands(device, transferCommandPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = srcBuffer.getSize();
    vkCmdCopyBuffer(commandBuffer.get(), srcBuffer.get(), dstBuffer.get(), 1, &copyRegion);

    CommandBuffer::endSingleTimeCommands(device, transferCommandPool, transferQueue, commandBuffer);
}


void Buffer::bindVertex(CommandBuffer commandBuffer) {
    #ifndef NDEBUG
        if (!(usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
            throw std::runtime_error("Trying to bind a buffer as a vertex buffer when it is not");
    #endif

    VkBuffer buffers[] = { buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer.get(), 0, 1, buffers, offsets);
}

void Buffer::bindIndex(CommandBuffer commandBuffer, VkIndexType indexType) {
    #ifndef NDEBUG
        if (!(usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
            throw std::runtime_error("Trying to bind a buffer as an index buffer when it is not");
    #endif

    vkCmdBindIndexBuffer(commandBuffer.get(), buffer, 0, indexType);
}

uint32_t Buffer::findMemoryType(PhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice.get(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type");
}
