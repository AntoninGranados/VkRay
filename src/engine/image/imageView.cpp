#include "imageView.hpp"

#include "image.hpp"
#include "../logicalDevice.hpp"
#include "../utils.hpp"

void ImageView::init(LogicalDevice device, Image image) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.get();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = image.getFormat();
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCheck(vkCreateImageView(device.get(), &viewInfo, nullptr, &imageView), "Failed to create the image view");
}

void ImageView::destroy(LogicalDevice device) {
    vkDestroyImageView(device.get(), imageView, nullptr);
}
