#include "engine.hpp"

// TODO: get this values from the application (and pass them in the engine.init)
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;

const uint32_t MAX_FRAME_IN_FLIGHT = 2; // Number of frames that can be worked on in parallele by the CPU

const std::vector<const char*> instanceExtensions = {
#ifdef __APPLE__
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
#ifndef NDEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
#ifdef __APPLE__
    "VK_KHR_portability_subset",
#endif
};

// only used if NDEBUG is ndef
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

/*
============== INITIALISATION ==============
*/

bool VkSmol::shouldTerminate() {
    return window.shouldClose();
}

void VkSmol::init(std::string appName, uint32_t appVersion) {
    glfwInit();

    window.init(WIDTH, HEIGHT, appName.c_str(), framebufferResizeCallback);

    // VkSmol informations
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName.c_str();
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = "VkSmol";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;
    
    instance.init(appInfo, getRequiredExtensions(), validationLayers);

    debugMessenger.init(instance);

    surface.init(instance, window);

    std::function<int(PhysicalDevice)> scoring = [this] (PhysicalDevice device) { return scoreDevice(device); };
    physicalDevice.init(instance, scoring);
    
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    device.init(
        physicalDevice,
        queueFamilyIndices.getIndices(),
        deviceExtensions,
        validationLayers
    );

    graphicsQueue.init(device, queueFamilyIndices.graphicsFamily.value());
    presentQueue.init(device, queueFamilyIndices.presentFamily.value());
    transferQueue.init(device, queueFamilyIndices.transferFamily.value());

    swapchain.init(device, physicalDevice, surface, window);
    
    dynamicRenderer.init(device);

    graphicsCommandPool.init(device, queueFamilyIndices.graphicsFamily.value());
    transferCommandPool.init(device, queueFamilyIndices.transferFamily.value());
    commandBuffers.resize(MAX_FRAME_IN_FLIGHT);
    for (CommandBuffer &commandBuffer : commandBuffers)
        commandBuffer.alloc(device, graphicsCommandPool);

    initImgui();

    // Synchronization Objects
    {
        imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);
        uiFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);
        renderInFlightFences.resize(MAX_FRAME_IN_FLIGHT);
        uiInFlightFences.resize(MAX_FRAME_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i].init(device);

            renderFinishedSemaphores[i].resize(swapchain.getImageCount());
            for (Semaphore &semaphore : renderFinishedSemaphores[i])
                semaphore.init(device);

            uiFinishedSemaphores[i].resize(swapchain.getImageCount());
            for (Semaphore &semaphore : uiFinishedSemaphores[i])
                semaphore.init(device);

            renderInFlightFences[i].init(device, true);
            uiInFlightFences[i].init(device, true);
        }
    }
}

void VkSmol::initImgui() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    uiCommandPool.init(device, indices.graphicsFamily.value());
    uiCommandBuffers.resize(MAX_FRAME_IN_FLIGHT);
    for (CommandBuffer &commandBuffer : uiCommandBuffers)
        commandBuffer.alloc(device, uiCommandPool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg]       = ImVec4(0.0f, 0.0f, 0.0f, 0.95f);
    ImGui::GetStyle().Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui_ImplGlfw_InitForVulkan(window.get(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance       = instance.get();
    initInfo.PhysicalDevice = physicalDevice.get();
    initInfo.Device         = device.get();
    initInfo.QueueFamily    = indices.graphicsFamily.value();
    initInfo.Queue          = graphicsQueue.get();
    
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000);
    uiDescriptorPool.addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100);
    uiDescriptorPool.init(device);
    
    // initInfo.RenderPass = uiRenderPass.get();
    VkPipelineRenderingCreateInfoKHR pipelineCreate{};
    pipelineCreate.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineCreate.colorAttachmentCount    = 1;
    pipelineCreate.pColorAttachmentFormats = swapchain.getImageFormat();
    
    initInfo.RenderPass                  = VK_NULL_HANDLE;
    initInfo.UseDynamicRendering         = true;
    initInfo.PipelineRenderingCreateInfo = pipelineCreate;
    
    initInfo.DescriptorPool = uiDescriptorPool.get();
    initInfo.MinImageCount  = 2;
    initInfo.ImageCount     = MAX_FRAME_IN_FLIGHT;
    initInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
}


GraphicsPipeline VkSmol::initGraphicsPipeline(
    VkPipelineVertexInputStateCreateInfo vertexInputInfo,
    std::vector<Shader> shaders,
    std::vector<DescriptorSetLayout> setLayouts,
    GraphicsPipeline other,
    VkFormat colorFormat
) {
    // Dynamic Rendering Info
    VkFormat format = colorFormat == VK_FORMAT_UNDEFINED ? *swapchain.getImageFormat() : colorFormat;
    VkPipelineRenderingCreateInfoKHR pipelineCreate{};
    pipelineCreate.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineCreate.colorAttachmentCount    = 1;
    pipelineCreate.pColorAttachmentFormats = &format;

    GraphicsPipeline pipeline;
    pipeline.init(
        device,
        pipelineCreate,
        vertexInputInfo,
        shaders,
        setLayouts,
        other
    );
    return pipeline;
}

void VkSmol::destroyGraphicsPipeline(GraphicsPipeline &pipeline) {
    pipeline.destroy(device);
}


Shader VkSmol::initShader(VkShaderStageFlagBits stage, std::string filepath) {
    Shader shader;
    shader.init(device, stage, filepath);
    return shader;
}

void VkSmol::destroyShader(Shader &shader) {
    shader.destroy(device);
}


void VkSmol::initDescriptorSetLayout(DescriptorSetLayout &descriptorSetLayout) {
    descriptorSetLayout.init(device);
}

void VkSmol::destroyDescriptorSetLayout(DescriptorSetLayout &descriptorSetLayout) {
    descriptorSetLayout.destroy(device);
}


bufferList_t VkSmol::initBufferList(VkBufferUsageFlags usage, size_t size) {
    //! it always uses persistent mapping
    std::vector<Buffer> bufferList;
    bufferList.resize(MAX_FRAME_IN_FLIGHT);
    for (Buffer &buffer : bufferList) {
        buffer.init(device, usage, size, true);
        buffer.alloc(device, physicalDevice, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    
    bufferLists.push_back(bufferList);
    return bufferLists.size()-1;
}

void VkSmol::destroyBufferList(bufferList_t bufferList) {
    for (Buffer &buffer : bufferLists[bufferList]) {
        buffer.free(device);
        buffer.destroy(device);
    }
}

Buffer VkSmol::getBuffer(bufferList_t bufferList) {
    return bufferLists[bufferList][currentFrame];
}


Buffer VkSmol::initBuffer(VkBufferUsageFlags usage, size_t size, void *data) {
    Buffer buffer;

    if (data == nullptr) {
        buffer.init(device, usage, size);
    } else {
        buffer.initFilled(
            device, physicalDevice,
            usage,
            transferCommandPool, transferQueue,
            size, data
        );
    }

    return buffer;
}

void VkSmol::destroyBuffer(Buffer &buffer) {
    buffer.free(device);
    buffer.destroy(device);
}

void VkSmol::fillBuffer(Buffer buffer, void *data) {
    buffer.fill(device, data);
}

descriptorSetList_t VkSmol::initDescriptorSetList(DescriptorSetLayout &layout, std::vector<void*> descriptors) {
    std::vector<VkDescriptorSetLayoutBinding> bindings = layout.getBindings();

    DescriptorPool descriptorPool;
    for (VkDescriptorSetLayoutBinding &binding : bindings)
        descriptorPool.addPoolSize(binding.descriptorType, MAX_FRAME_IN_FLIGHT);
    descriptorPool.init(device);

    std::vector<DescriptorSet> descriptorSetList;
    descriptorSetList.resize(MAX_FRAME_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        descriptorSetList[i].alloc(device, descriptorPool, layout);

        for (size_t bind_idx = 0; bind_idx < bindings.size(); bind_idx++) {
            switch (bindings[bind_idx].descriptorType) {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                    bufferList_t bufferList = *(bufferList_t*)descriptors[bind_idx];
                    descriptorSetList[i].linkUniformBuffer(bufferLists[bufferList][i], bindings[bind_idx].binding);
                } break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    bufferList_t bufferList = *(bufferList_t*)descriptors[bind_idx];
                    descriptorSetList[i].linkStorageBuffer(bufferLists[bufferList][i], bindings[bind_idx].binding);
                } break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                    std::pair<ImageView, Sampler> pair = *(std::pair<ImageView, Sampler>*)descriptors[bind_idx];
                    descriptorSetList[i].linkCombinedImageSampler(pair.first, pair.second, bindings[bind_idx].binding);
                } break;
                default:
                    throw std::runtime_error("Unsupported descriptor type");
            }
        }

        descriptorSetList[i].update(device);
    }
    
    descriptorPools.push_back(descriptorPool);
    descriptorSetLists.push_back(descriptorSetList);
    return descriptorSetLists.size()-1;
}

DescriptorSet VkSmol::getDescriptorSet(descriptorSetList_t descriptorSetList) {
    return descriptorSetLists[descriptorSetList][currentFrame];
}


Image VkSmol::initImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
    Image image;
    image.init(device, width, height, format, usage);
    image.alloc(device, physicalDevice, properties);
    return image;
}

Image VkSmol::initImageFromPath(std::string path) {
    Image image;
    image.initFromPath(device, physicalDevice, transferCommandPool, transferQueue, path);
    return image;
}

ImageView VkSmol::initImageView(Image &image) {
    ImageView imageView;
    imageView.init(device, image);
    return imageView;
}

void VkSmol::destroyImage(Image &image) {
    image.free(device);
    image.destroy(device);
}

void VkSmol::destroyImageView(ImageView &imageView) {
    imageView.destroy(device);
}


Sampler VkSmol::initSampler() {
    Sampler sampler;
    sampler.init(device, physicalDevice);
    return sampler;
}

void VkSmol::destroySampler(Sampler &sampler) {
    sampler.destroy(device);
}



void VkSmol::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    VkSmol* pApp = reinterpret_cast<VkSmol*>(glfwGetWindowUserPointer(window));
    pApp->framebufferResized = true;
}

std::vector<const char*> VkSmol::getRequiredExtensions() {
    // Extensions needed by GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Extensions requiMSG_RED by the application
    std::vector<const char*> requiredExtensions;
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
        requiredExtensions.emplace_back(glfwExtensions[i]);
    requiredExtensions.insert(requiredExtensions.end(), instanceExtensions.begin(), instanceExtensions.end());
    
    return requiredExtensions;
}

int VkSmol::scoreDevice(PhysicalDevice device) {
    // Check if all the  queue families are available
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete())
        return 0;

    // Check if all extensions are supported by the GPU
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device.get(), nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device.get(), nullptr, &extensionCount, supportedExtensions.data());

    for (const char* extension : deviceExtensions) {
        if (!std::any_of(
            supportedExtensions.begin(),
            supportedExtensions.end(),
            [&](VkExtensionProperties &properties) { return strcmp(extension, properties.extensionName) == 0; })
        )
            return 0;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device.get(), &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy)
        return 0;

    // Check swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return 0;
    
    return 1;
}


void VkSmol::beginFrame() {
    glfwPollEvents();

    Fence::wait({ renderInFlightFences[currentFrame], uiInFlightFences[currentFrame] }, device);

    uint32_t result;
    result = swapchain.getNextImageIndex(device, imageAvailableSemaphores[currentFrame]);
    if (result == RECREATE_SWAPCHAIN) {
        swapchain.recreate(device, physicalDevice, surface, window);
        return;
    }

    Fence::reset({ renderInFlightFences[currentFrame], uiInFlightFences[currentFrame] }, device);
    imageIndex = result;
}

void VkSmol::endFrame() {
    uint32_t result = presentQueue.present({ swapchain }, { imageIndex }, { uiFinishedSemaphores[currentFrame][imageIndex] });
    if (result == RECREATE_SWAPCHAIN || framebufferResized) {
        framebufferResized = false;
        swapchain.recreate(device, physicalDevice, surface, window);
    }

    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;

    window.pauseWhileMinimized();
}

CommandBuffer VkSmol::beginRecordingRender() {
    CommandBuffer commandBuffer = commandBuffers[currentFrame];
    commandBuffer.reset();

    commandBuffer.beginRecording();
    return commandBuffer;
}

void VkSmol::endRecoringRender(CommandBuffer commandBuffer) {
    commandBuffer.endRecording();

    graphicsQueue.submit(
        { commandBuffers[currentFrame] },
        renderInFlightFences[currentFrame],
        { imageAvailableSemaphores[currentFrame] }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
        { renderFinishedSemaphores[currentFrame][imageIndex] }
    );
}

CommandBuffer VkSmol::beginRecordingUiRender() {
    CommandBuffer commandBuffer = uiCommandBuffers[currentFrame];
    commandBuffer.reset();
    
    commandBuffer.beginRecording();

    return commandBuffer;
}

void VkSmol::endRecoringUiRender(CommandBuffer commandBuffer) {
    commandBuffer.endRecording();   
    graphicsQueue.submit(
        { uiCommandBuffers[currentFrame] },
        uiInFlightFences[currentFrame],
        { renderFinishedSemaphores[currentFrame][imageIndex] }, { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT },
        { uiFinishedSemaphores[currentFrame][imageIndex] }
    );
}


//! The render area is fixed to the screen size for now
void VkSmol::beginDynamicRenderer(
    CommandBuffer commandBuffer, 
    VkImageView imageView, VkImageLayout imageLayout,
    VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
    VkClearColorValue clearColor
) {
    if (imageView == VK_NULL_HANDLE)
        imageView = swapchain.getImageView(imageIndex);

    dynamicRenderer.begin(
        commandBuffer,
        imageView, imageLayout,
        loadOp, storeOp,
        clearColor, VkRect2D { VkOffset2D {}, swapchain.getExtent() }
    );
}

void VkSmol::endDynamicRenderer(CommandBuffer commandBuffer) {
    dynamicRenderer.end(commandBuffer);
}

// Will use the barrier on the swapchain image if `image` is `nullptr`
void VkSmol::barrier(
    CommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout srcLayout, VkImageLayout dstLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask
) {
    ImageBarrier barrier;
    barrier.call(
        commandBuffer,
        image == nullptr ? swapchain.getImage(imageIndex) : image,
        srcLayout, dstLayout,
        srcAccessMask, dstAccessMask,
        srcStageMask, dstStageMask
    );
}


void VkSmol::waitIdle() {
    device.waitIdle();
}

void VkSmol::terminate() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    for(DescriptorPool &descriptorPool : descriptorPools)
        descriptorPool.destroy(device);
    uiDescriptorPool.destroy(device);

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        imageAvailableSemaphores[i].destroy(device);
        for (Semaphore &semaphore : renderFinishedSemaphores[i])
            semaphore.destroy(device);
        for (Semaphore &semaphore : uiFinishedSemaphores[i])
            semaphore.destroy(device);
        renderInFlightFences[i].destroy(device);
        uiInFlightFences[i].destroy(device);
    }

    for (CommandBuffer &commandBuffer : commandBuffers)
        commandBuffer.free(device, graphicsCommandPool);
    for (CommandBuffer &commandBuffer : uiCommandBuffers)
        commandBuffer.free(device, uiCommandPool);
    graphicsCommandPool.destroy(device);
    transferCommandPool.destroy(device);
    uiCommandPool.destroy(device);

    swapchain.destroy(device);

    device.destroy();
    surface.destroy(instance);

    debugMessenger.destroy(instance);
    instance.destroy();

    window.destroy();
    glfwTerminate();
}
