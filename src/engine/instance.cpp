#include "instance.hpp"

#include "./debugMessenger.hpp"
#include "./utils.hpp"

void Instance::init(
    VkApplicationInfo appInfo,
    std::vector<const char*> extensions,
    std::vector<const char*> validationLayers
) {
    // Extra checks in debug mode
    #ifndef NDEBUG
        Instance::checkExtensionSupport(extensions);
        Instance::checkValidationLayerSupport(validationLayers);
    #endif

    // Fill the creation info struct
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    #ifndef NDEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // to debug the `vkCreateInstance` and `vkDestroyInstance` functions
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        DebugMessenger::populateCreateInfo(debugMessengerCreateInfo);
        createInfo.pNext = &debugMessengerCreateInfo;
    #else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    #endif
    #ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    vkCheck(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create instance");
}

void Instance::destroy() {
    vkDestroyInstance(instance, nullptr);
}


void Instance::checkExtensionSupport(std::vector<const char*> extensions) {
    // Find all the supported extensions
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());
    
    // Check if all extensions in `extensions` are supported
    for (const char* extension : extensions) {
        if (!std::any_of(
            supportedExtensions.begin(),
            supportedExtensions.end(),
            [&](VkExtensionProperties& properties) { return strcmp(extension, properties.extensionName) == 0; })
        ) {
            char buf[MAX_MSG_SIZE];
            snprintf(buf, sizeof(buf), "Instance Extension [%s%s%s] is not supported", MSG_RED_B, extension, MSG_RESET);
            throw std::runtime_error(buf);
        }
    }
}

void Instance::checkValidationLayerSupport(std::vector<const char*> validationLayers) {
    // Find all the supported validation layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> supportedLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());

    // Check if all validation layers in `validationLayers` are supported
    for (const char* layer : validationLayers) {
        if (!std::any_of(
            supportedLayers.begin(),
            supportedLayers.end(),
            [&](VkLayerProperties& properties) { return strcmp(layer, properties.layerName) == 0; })
        ) {
            char buf[MAX_MSG_SIZE];
            std::snprintf(buf, sizeof(buf), "Validation Layer [%s%s%s] is not supported", MSG_RED_B, layer, MSG_RESET);
            throw std::runtime_error(buf);
        }
    }
}