#pragma once

#include <vulkan/vulkan.h>

#include <optional>
#include <algorithm>
#include <vector>

class Surface;
class PhysicalDevice;
class LogicalDevice;
class CommandBuffer;
class Swapchain;
class Semaphore;
class Fence;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // queue for graphics
    std::optional<uint32_t> presentFamily;  // queue for prensenting surface to the screen
    std::optional<uint32_t> transferFamily; // queue for transfering data from the CPU to the GPU

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
    }

    // Check if all the families are the same
    bool sameFamilies() {
        return std::ranges::all_of(getIndices(), [&](int i) { return i == getIndices()[0]; });
    }

    std::vector<uint32_t> getIndices() {
        return std::vector<uint32_t> { graphicsFamily.value(), presentFamily.value(), transferFamily.value() };
    }
};

QueueFamilyIndices findQueueFamilies(PhysicalDevice physicalDevice, Surface surface);

class Queue {
public:
    void init(LogicalDevice device, uint32_t queueFamilyIndex);
    // void destroy();
    VkQueue get() { return queue; };

    // Should be able to generate multiple VkSubmitInfo and submit them
    void submit(
        std::vector<CommandBuffer> commandBuffers,
        Fence fence,
        std::vector<Semaphore> waitSemaphores, std::vector<VkPipelineStageFlags> waitStages,
        std::vector<Semaphore> signalSemaphores
    );
    void submit(std::vector<CommandBuffer> commandBuffers);
    // Returns RECREATE_SWAPCHAIN if we need to recreate the swapchain
    uint32_t present(
        std::vector<Swapchain> swapchains,
        std::vector<uint32_t> frameIndices,
        std::vector<Semaphore> waitSemaphores
    );

    void wait();

private:
    VkQueue queue;
};
