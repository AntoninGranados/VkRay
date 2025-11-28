#pragma once

#include <vulkan/vulkan.h>

class LogicalDevice;
class Image;

class ImageView {
public:
    void init(LogicalDevice device, Image image);
    void destroy(LogicalDevice device);

    VkImageView get() { return imageView; }

private:
    VkImageView imageView;
};
