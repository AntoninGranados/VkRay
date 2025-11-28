#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <vector>

class Instance {
public:
    void init(
        VkApplicationInfo appInfo,
        std::vector<const char*> extensions,
        std::vector<const char*> validationLayers
    );
    void destroy();
    VkInstance get() { return instance; }


private:
    VkInstance instance;

    static void checkExtensionSupport(std::vector<const char*> extensions);
    static void checkValidationLayerSupport(std::vector<const char*> validationLayers);
};
