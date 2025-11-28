#pragma once

#include <vulkan/vulkan.h>

class Instance;

class DebugMessenger {
public:
    void init(Instance instance);
    void destroy(Instance instance);
    VkDebugUtilsMessengerEXT get() { return messenger; };

    static void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
    VkDebugUtilsMessengerEXT messenger;

    static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    // We need this proxy function because we have to load `vkCreateDebugUtilsMessengerEXT` manually
    static VkResult CreateDebugUtilsMessengerEXT(
        Instance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );

    static void DestroyDebugUtilsMessengerEXT(Instance instance,
        VkDebugUtilsMessengerEXT debugMessenger, const
        VkAllocationCallbacks* pAllocator
    );
};