#include "swapchain.hpp"

#include "./window.hpp"
#include "./physicalDevice.hpp"
#include "./logicalDevice.hpp"
#include "./queue.hpp"
#include "./surface.hpp"
#include "./sync/semaphore.hpp"
#include "./utils.hpp"

SwapChainSupportDetails querySwapChainSupport(PhysicalDevice device, Surface surface) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.get(), surface.get(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.get(), surface.get(), &formatCount, nullptr);
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.get(), surface.get(), &formatCount, details.formats.data());
    }
    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.get(), surface.get(), &presentModeCount, nullptr);
    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.get(), surface.get(), &presentModeCount, details.presentModes.data());
    }

    return details;
}


void Swapchain::init(LogicalDevice device, PhysicalDevice physicalDevice, Surface surface, Window window, std::optional<Swapchain> oldSwapChain) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    extent = chooseSwapExtent(swapChainSupport.capabilities, window.get());
    imageFormat = surfaceFormat.format;

    imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount != 0)
        imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface.get();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // If we want to do post processing, we should use `VK_IMAGE_USAGE_TRANSFER_DST_BIT`

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    // In the case where the two families are different
    // we have to tell the GPU that the images will be shared accross mulitple families
    if (!indices.sameFamilies()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.getIndices().size());
        createInfo.pQueueFamilyIndices = indices.getIndices().data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;   // No added transformation
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // For an opaque window
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;   // Pixel behind other window will be clipped
    createInfo.oldSwapchain = oldSwapChain == std::nullopt ? VK_NULL_HANDLE : oldSwapChain.value().get();

    vkCheck(vkCreateSwapchainKHR(device.get(), &createInfo, nullptr, &swapchain), "Failed to create the swap chain");

    initImages(device);
    initImageViews(device);
}

void Swapchain::destroy(LogicalDevice device) {
    destroyImageViews(device);
    vkDestroySwapchainKHR(device.get(), swapchain, nullptr);

    images.clear();
    imageViews.clear();
}

// TODO: recreate the swapchain using the old one to remove the waitIdle call (would need to defer the destruction of the old one)
void Swapchain::recreate(LogicalDevice device, PhysicalDevice physicalDevice, Surface surface, Window window) {
    device.waitIdle();

    destroy(device);

    init(device, physicalDevice, surface, window);
}

uint32_t Swapchain::getNextImageIndex(LogicalDevice device, Semaphore semaphore) {
    uint32_t frameIndex;

    VkResult result = vkAcquireNextImageKHR(device.get(), swapchain, UINT64_MAX, semaphore.get(), VK_NULL_HANDLE, &frameIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
        return RECREATE_SWAPCHAIN;
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swapchain image");

    return frameIndex;
}



void Swapchain::initImages(LogicalDevice device) {
    vkGetSwapchainImagesKHR(device.get(), swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device.get(), swapchain, &imageCount, images.data());
}

void Swapchain::initImageViews(LogicalDevice device) {
    imageViews.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;
        
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        vkCheck(vkCreateImageView(device.get(), &createInfo, nullptr, &imageViews[i]), "Failed to create image views");
    }
}

void Swapchain::destroyImageViews(LogicalDevice device) {
    for (size_t i = 0; i < imageCount; i++) {
        vkDestroyImageView(device.get(), imageViews[i], nullptr);
    }
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Find if the SRGB non-linear color space and that BRGA (8bit) format are available
    for (const VkSurfaceFormatKHR& format : availableFormats) {
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB)
            return format;
    }

    // Settle for the first one if our prefered color space/format are not available
    return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Find if the mailbox present mode is available
    // Mailbox : display the images as fast as possible while avoiding tearing
    for (const VkPresentModeKHR& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    }

    // This mode is guaranted
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}
