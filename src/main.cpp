#include "meshLoader.h"
#include <cstdint>
#include <cstdio>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "array"
#include "common.h"
#include "iostream"
#include "vector"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vma.h"

#include "common.h"
#include "types.h"

static VkBool32 message(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cerr << "[ERROR] ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "[WARNING] ";
    }

    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        std::cerr << "General: ";
    }
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        std::cerr << "Validation: ";
    }
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        std::cerr << "Performance: ";
    }

    std::cerr << pCallbackData->pMessage << std::endl << std::endl;
    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT getMessengerCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = message;

    return messengerCreateInfo;
}

static VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance)
{
    if (!instance)
    {
        return nullptr;
    }

    using MessengerFuncPtr = PFN_vkCreateDebugUtilsMessengerEXT;
    MessengerFuncPtr createDebugMessagerFunc = (MessengerFuncPtr)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!createDebugMessagerFunc)
    {
        std::cerr << "Could not find proc addr vkCreateDebugUtilsMessengerEXT" << std::endl;
        return nullptr;
    }

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = getMessengerCreateInfo();
    VkDebugUtilsMessengerEXT messenger = nullptr;
    VK_LOG_ERR(createDebugMessagerFunc(instance, &messengerCreateInfo, nullptr, &messenger));
    return messenger;
}

static void destroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
{
    if (!instance || !messenger)
    {
        return;
    }

    using MessengerFuncPtr = PFN_vkDestroyDebugUtilsMessengerEXT;
    MessengerFuncPtr destroyDebugMessagerFunc = (MessengerFuncPtr)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!destroyDebugMessagerFunc)
    {
        std::cerr << "Could not find proc addr vkDestroyDebugUtilsMessengerEXT" << std::endl;
        return;
    }

    destroyDebugMessagerFunc(instance, messenger, nullptr);
}

static VkInstance createInstance()
{
    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

    uint32_t layerCount = 0;
    VK_LOG_ERR(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK_LOG_ERR(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Para-ray";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t requiredExtensionCount = 0;
    const char **requiredExtensions_cstr = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<const char *> requiredExtensions = std::vector<const char *>(requiredExtensions_cstr, requiredExtensions_cstr + requiredExtensionCount);

#ifndef NDEBUG
    static const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
    static const char *debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = getMessengerCreateInfo();
    requiredExtensions.push_back(debugExtension);

    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
    instanceCreateInfo.pNext = &messengerCreateInfo;
#endif

    instanceCreateInfo.enabledExtensionCount = requiredExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkInstance instance = nullptr;
    VK_LOG_ERR(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
    return instance;
}

static PhysDeviceInfo getPhysicalDevice(VkInstance instance)
{
    if (!instance)
    {
        return {};
    }

    uint32_t numPhysicalDevices = 0;
    VK_LOG_ERR(vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    VK_LOG_ERR(vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices.data()));

    for (auto &physDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physDevice, &properties);

        // grab the first discrete gpu
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            std::cout << "Picking " << properties.deviceName << std::endl;

            PhysDeviceInfo deviceInfo = {};
            deviceInfo.device = physDevice;
            deviceInfo.deviceProperties = properties;

            vkGetPhysicalDeviceFeatures(physDevice, &deviceInfo.deviceFeatures);

            uint32_t numQueueProperties = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &numQueueProperties, nullptr);
            deviceInfo.queueProperties = std::vector<VkQueueFamilyProperties>(numQueueProperties);
            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &numQueueProperties, deviceInfo.queueProperties.data());

            return deviceInfo;
        }
    }
    return {};
}

static WindowInfo createWindow(VkInstance instance, const PhysDeviceInfo &physDeviceInfo)
{
    if (!instance || !physDeviceInfo.device)
    {
        return {};
    }
    WindowInfo windowInfo = {};

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    windowInfo.window = glfwCreateWindow(width, height, "Pararay", nullptr, nullptr);

    VK_LOG_ERR(glfwCreateWindowSurface(instance, windowInfo.window, nullptr, &windowInfo.surface));

    uint32_t surfaceFormatCount = 0;
    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(physDeviceInfo.device, windowInfo.surface, &surfaceFormatCount, nullptr));
    windowInfo.surfaceFormats = std::vector<VkSurfaceFormatKHR>(surfaceFormatCount);
    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(physDeviceInfo.device, windowInfo.surface, &surfaceFormatCount, windowInfo.surfaceFormats.data()));

    bool foundPreferredSurfaceFormat = false;
    for (auto &surfaceFormat : windowInfo.surfaceFormats)
    {
        if (surfaceFormat.format == windowInfo.preferredSurfaceFormat.format && surfaceFormat.colorSpace == windowInfo.preferredSurfaceFormat.colorSpace)
        {
            foundPreferredSurfaceFormat = true;
            break;
        }
    }
    if (!foundPreferredSurfaceFormat)
    {
        std::cerr << "Preferred surface format not available" << std::endl;
        return {};
    }

    uint32_t presentModeCount = 0;
    VK_LOG_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(physDeviceInfo.device, windowInfo.surface, &presentModeCount, nullptr));
    windowInfo.presentModes = std::vector<VkPresentModeKHR>(presentModeCount);
    VK_LOG_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(physDeviceInfo.device, windowInfo.surface, &presentModeCount, windowInfo.presentModes.data()));

    bool foundPreferredPresentMode = false;
    for (auto &presentMode : windowInfo.presentModes)
    {
        if (presentMode == windowInfo.preferredPresentMode)
        {
            foundPreferredPresentMode = true;
            break;
        }
    }
    if (!foundPreferredPresentMode)
    {
        std::cerr << "Preferred present mode not available" << std::endl;
        return {};
    }

    std::cout << std::endl;

    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDeviceInfo.device, windowInfo.surface, &windowInfo.surfaceCapabilities));

    return windowInfo;
}

static DeviceInfo createDevice(VkInstance instance, const PhysDeviceInfo &physDeviceInfo, const WindowInfo &windowInfo)
{
    if (!instance || !physDeviceInfo.device)
    {
        return {};
    }

    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(physDeviceInfo.queueProperties.size());
    for (uint32_t i = 0; i < queueCreateInfos.size(); i++)
    {
        auto &createInfo = queueCreateInfos[i];
        const auto &queueProperties = physDeviceInfo.queueProperties[i];
        constexpr float queuePriority = 1.0f;

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueCount = 1;
        createInfo.queueFamilyIndex = i;
        createInfo.pQueuePriorities = &queuePriority;
    }

    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &physDeviceInfo.deviceFeatures;

    std::array<const char *, 1> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceCreateInfo.enabledExtensionCount = requiredExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    DeviceInfo deviceInfo = {};
    VK_LOG_ERR(vkCreateDevice(physDeviceInfo.device, &deviceCreateInfo, nullptr, &deviceInfo.device));

    deviceInfo.queues.resize(physDeviceInfo.queueProperties.size());
    for (uint32_t i = 0; i < physDeviceInfo.queueProperties.size(); i++)
    {
        deviceInfo.queues[i].queueFamilyIdx = i;
        deviceInfo.queues[i].queueIdx = 0;
        deviceInfo.queues[i].queueProperties = physDeviceInfo.queueProperties[i];
        vkGetDeviceQueue(deviceInfo.device, i, 0, &deviceInfo.queues[i].queue);
    }

    for (uint32_t i = 0; i < deviceInfo.queues.size(); i++)
    {
        if (deviceInfo.queues[i].queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            deviceInfo.graphicsQueueFamily = i;
            deviceInfo.transferQueueFamily = i;
            break;
        }
    }

    for (uint32_t i = 0; i < deviceInfo.queues.size(); i++)
    {
        if (deviceInfo.queues[i].queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(deviceInfo.queues[i].queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            deviceInfo.transferQueueFamily = i;
            break;
        }
    }

    for (uint32_t i = 0; i < deviceInfo.queues.size(); i++)
    {
        VkBool32 presentSupport = VK_FALSE;
        VK_LOG_ERR(vkGetPhysicalDeviceSurfaceSupportKHR(physDeviceInfo.device, i, windowInfo.surface, &presentSupport));

        if (presentSupport == VK_TRUE)
        {
            deviceInfo.presentQueueFamily = i;
            break;
        }
    }

    return deviceInfo;
}

static SwapchainInfo createSwapchain(DeviceInfo &deviceInfo, const WindowInfo &windowInfo, VkSwapchainKHR oldSwapchain = nullptr)
{
    if (!deviceInfo.device || !windowInfo.window || !windowInfo.surface)
    {
        return {};
    }

    int width;
    int height;
    glfwGetWindowSize(windowInfo.window, &width, &height);
    std::cout << width << " " << height << std::endl;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.oldSwapchain = oldSwapchain;
    swapchainCreateInfo.imageFormat = windowInfo.preferredSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = windowInfo.preferredSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.minImageCount =
        numPreferredImages > windowInfo.surfaceCapabilities.maxImageCount ? windowInfo.surfaceCapabilities.maxImageCount : numPreferredImages;
    swapchainCreateInfo.presentMode = windowInfo.preferredPresentMode;
    swapchainCreateInfo.surface = windowInfo.surface;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    std::vector<uint32_t> queueFamilyIndices;
    for (const auto &queueInfo : deviceInfo.queues)
    {
        if (queueInfo.queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndices.push_back(queueInfo.queueFamilyIdx);
        }
    }

    swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
    swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    SwapchainInfo swapchainInfo = {};
    swapchainInfo.numImages = swapchainCreateInfo.minImageCount;
    swapchainInfo.height = swapchainCreateInfo.imageExtent.height;
    swapchainInfo.width = swapchainCreateInfo.imageExtent.width;
    VK_LOG_ERR(vkCreateSwapchainKHR(deviceInfo.device, &swapchainCreateInfo, nullptr, &swapchainInfo.swapchain));

    return swapchainInfo;
}

static VkRenderPass createRenderPass(const DeviceInfo &deviceInfo, const WindowInfo &windowInfo)
{
    if (!deviceInfo.device || !windowInfo.window || !windowInfo.surface)
    {
        return nullptr;
    }

    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescription.format = windowInfo.preferredSurfaceFormat.format;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkRenderPassCreateInfo renderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;

    VkRenderPass renderPass = nullptr;
    VK_LOG_ERR(vkCreateRenderPass(deviceInfo.device, &renderPassCreateInfo, nullptr, &renderPass));
    return renderPass;
}

static VkPipelineLayout createPipelineLayout(const DeviceInfo &deviceInfo)
{
    if (!deviceInfo.device)
    {
        return nullptr;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout pipelineLayout = nullptr;
    VK_LOG_ERR(vkCreatePipelineLayout(deviceInfo.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

static ImageGroupInfo getSwapChainImages(const DeviceInfo &deviceInfo, const WindowInfo &windowInfo, const SwapchainInfo &swapchainInfo,
                                         const VkRenderPass renderPass)
{
    if (!deviceInfo.device || !windowInfo.window || !windowInfo.surface || !swapchainInfo.swapchain)
    {
        return {};
    }

    ImageGroupInfo swapchainImages = {};

    uint32_t swapchainImageCount = 0;
    VK_LOG_ERR(vkGetSwapchainImagesKHR(deviceInfo.device, swapchainInfo.swapchain, &swapchainImageCount, nullptr));
    swapchainImages.images = std::vector<VkImage>(swapchainImageCount);
    swapchainImages.imageViews = std::vector<VkImageView>(swapchainImageCount);
    swapchainImages.frameBuffers = std::vector<VkFramebuffer>(swapchainImageCount);
    VK_LOG_ERR(vkGetSwapchainImagesKHR(deviceInfo.device, swapchainInfo.swapchain, &swapchainImageCount, swapchainImages.images.data()));

    VkImageSubresourceRange subResourceRange;
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.layerCount = 1;
    subResourceRange.levelCount = 1;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.image = swapchainImages.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = windowInfo.preferredSurfaceFormat.format;
        imageViewCreateInfo.subresourceRange = subResourceRange;

        VK_LOG_ERR(vkCreateImageView(deviceInfo.device, &imageViewCreateInfo, nullptr, &swapchainImages.imageViews[i]));
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkFramebufferCreateInfo framebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &swapchainImages.imageViews[i];
        framebufferCreateInfo.width = swapchainInfo.width;
        framebufferCreateInfo.height = swapchainInfo.height;
        framebufferCreateInfo.layers = 1;

        VK_LOG_ERR(vkCreateFramebuffer(deviceInfo.device, &framebufferCreateInfo, nullptr, &swapchainImages.frameBuffers[i]));
    }

    return swapchainImages;
}

void freeSwapchainImages(const VkDevice device, ImageGroupInfo &swapchainImages)
{
    for (auto &fb : swapchainImages.frameBuffers)
    {
        vkDestroyFramebuffer(device, fb, nullptr);
    }
    for (auto &iv : swapchainImages.imageViews)
    {
        vkDestroyImageView(device, iv, nullptr);
    }
}

static VkCommandPool createCommandPool(const DeviceInfo &deviceInfo)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = deviceInfo.graphicsQueueFamily;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool commandPool = nullptr;
    VK_LOG_ERR(vkCreateCommandPool(deviceInfo.device, &commandPoolCreateInfo, nullptr, &commandPool));
    return commandPool;
}

struct RenderContext
{
    std::vector<VkSemaphore> imgAvailableSem;
    std::vector<VkSemaphore> renderDoneSem;
    std::vector<VkFence> fences;
    std::vector<VkCommandPool> commandPools;
    std::vector<VkCommandBuffer> commandBuffers;
};

VkCommandBuffer createCommandBuffer(const DeviceInfo &deviceInfo, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer commandBuffer = nullptr;
    VK_LOG_ERR(vkAllocateCommandBuffers(deviceInfo.device, &commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
}

static RenderContext createRenderContext(const DeviceInfo &deviceInfo, const SwapchainInfo &swapchainInfo)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    RenderContext renderContext = {};
    renderContext.commandPools.resize(swapchainInfo.numImages);
    renderContext.commandBuffers.resize(swapchainInfo.numImages);
    renderContext.imgAvailableSem.resize(swapchainInfo.numImages);
    renderContext.renderDoneSem.resize(swapchainInfo.numImages);
    renderContext.fences.resize(swapchainInfo.numImages);

    for (uint32_t i = 0; i < swapchainInfo.numImages; i++)
    {
        renderContext.commandPools[i] = createCommandPool(deviceInfo);
        renderContext.commandBuffers[i] = createCommandBuffer(deviceInfo, renderContext.commandPools[i]);
        VK_LOG_ERR(vkCreateSemaphore(deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.imgAvailableSem[i]));
        VK_LOG_ERR(vkCreateSemaphore(deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.renderDoneSem[i]));
        VK_LOG_ERR(vkCreateFence(deviceInfo.device, &fenceCreateInfo, nullptr, &renderContext.fences[i]));
    }

    return renderContext;
}

void destroyRenderContext(const VkDevice device, RenderContext &renderContext)
{
    for (unsigned i = 0; i < renderContext.commandBuffers.size(); i++)
    {
        auto &cmdBuf = renderContext.commandBuffers[i];
        vkFreeCommandBuffers(device, renderContext.commandPools[i], 1, &cmdBuf);
    }
    for (auto &cmdPool : renderContext.commandPools)
    {
        vkDestroyCommandPool(device, cmdPool, nullptr);
    }
    for (auto &sem : renderContext.imgAvailableSem)
    {
        vkDestroySemaphore(device, sem, nullptr);
    }
    for (auto &sem : renderContext.renderDoneSem)
    {
        vkDestroySemaphore(device, sem, nullptr);
    }
    for (auto &fence : renderContext.fences)
    {
        vkDestroyFence(device, fence, nullptr);
    }
}

VkResult drawFrame(const DeviceInfo &deviceInfo, const SwapchainInfo &swapchainInfo, const RenderContext &renderContext, const ImageGroupInfo &swapChainImages,
                   VkRenderPass renderPass, uint32_t frameNum, VkPipeline pipeline, const WindowInfo &window)
{
    uint32_t frameIdx = frameNum % swapchainInfo.numImages;

    VK_LOG_ERR(vkWaitForFences(deviceInfo.device, 1, &renderContext.fences[frameIdx], VK_TRUE, UINT64_MAX));
    VK_LOG_ERR(vkResetFences(deviceInfo.device, 1, &renderContext.fences[frameIdx]));

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = {swapchainInfo.width, swapchainInfo.height};
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = renderPass;

    VkClearValue clearValue = {0.4f, 0.4f, 0.4f, 1.0f};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window.window, &windowWidth, &windowHeight);

    VkViewport viewport;
    viewport.height = windowHeight;
    viewport.width = windowWidth;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)windowWidth, (uint32_t)windowHeight};

    renderPassBeginInfo.framebuffer = swapChainImages.frameBuffers[frameIdx];

    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_LOG_ERR(vkBeginCommandBuffer(renderContext.commandBuffers[frameIdx], &commandBufferBeginInfo));
    vkCmdSetViewport(renderContext.commandBuffers[frameIdx], 0, 1, &viewport);
    vkCmdSetScissor(renderContext.commandBuffers[frameIdx], 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(renderContext.commandBuffers[frameIdx], &renderPassBeginInfo, subpassContents);
    vkCmdBindPipeline(renderContext.commandBuffers[frameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(renderContext.commandBuffers[frameIdx], 3, 1, 0, 0);
    vkCmdEndRenderPass(renderContext.commandBuffers[frameIdx]);
    VK_LOG_ERR(vkEndCommandBuffer(renderContext.commandBuffers[frameIdx]));

    uint32_t imageIndex = 0;
    VK_LOG_ERR(vkAcquireNextImageKHR(deviceInfo.device, swapchainInfo.swapchain, UINT64_MAX, renderContext.imgAvailableSem[frameIdx], nullptr, &imageIndex));

    VkPipelineStageFlags waitDstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &renderContext.imgAvailableSem[frameIdx];
    submitInfo.pWaitDstStageMask = &waitDstStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderContext.commandBuffers[frameIdx];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderContext.renderDoneSem[frameIdx];

    VK_LOG_ERR(vkQueueSubmit(deviceInfo.queues[deviceInfo.graphicsQueueFamily].queue, 1, &submitInfo, renderContext.fences[frameIdx]));

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderContext.renderDoneSem[frameIdx];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainInfo.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(deviceInfo.queues[deviceInfo.presentQueueFamily].queue, &presentInfo);
}

VkShaderModule loadShaderModule(const VkDevice device, const char *filePath)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return nullptr;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderModuleCreateInfo.codeSize = fileSize;
    shaderModuleCreateInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    VK_LOG_ERR(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));
    return shaderModule;
}

VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(VkShaderModule shader, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipelineShaderStageCreateInfo.stage = stage;
    pipelineShaderStageCreateInfo.module = shader;
    pipelineShaderStageCreateInfo.pName = "main";

    return pipelineShaderStageCreateInfo;
}

VkPipeline createPipeline(const VkDevice device, const VkRenderPass renderPass, const VkPipelineLayout pipelineLayout,
                          const std::vector<VkPipelineShaderStageCreateInfo> &shaders)
{
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tesselationCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = nullptr;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterCreateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterCreateInfo.lineWidth = 1.0f;
    rasterCreateInfo.depthBiasEnable = VK_FALSE;
    rasterCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterCreateInfo.depthBiasClamp = 0.0f;
    rasterCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo MSAACreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MSAACreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    MSAACreateInfo.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilCreateInfo.depthTestEnable = VK_FALSE;
    depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = 0.0f;
    depthStencilCreateInfo.maxDepthBounds = 1.0f;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateCreateInfo.dynamicStateCount = ARR_CNT(dynamicStates);
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stageCount = shaders.size();
    pipelineCreateInfo.pStages = shaders.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pTessellationState = &tesselationCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterCreateInfo;
    pipelineCreateInfo.pMultisampleState = &MSAACreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineHandle = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));
    return pipeline;
}

VmaAllocator createVmaAllocator(const VkInstance instance, const VkPhysicalDevice physicalDevice, const VkDevice device)
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    VK_LOG_ERR(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
    return allocator;
}

TransferQueue createTransferQueue(const DeviceInfo &deviceInfo)
{
    TransferQueue transferQueue = {};

    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = deviceInfo.transferQueueFamily;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool transferPool;
    VK_LOG_ERR(vkCreateCommandPool(deviceInfo.device, &commandPoolCreateInfo, nullptr, &transferPool));

    VkCommandBufferAllocateInfo commandBufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocInfo.commandPool = transferPool;
    commandBufferAllocInfo.commandBufferCount = 2;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cmdBufs[2] = {};
    VK_LOG_ERR(vkAllocateCommandBuffers(deviceInfo.device, &commandBufferAllocInfo, cmdBufs));

    transferQueue.commandPool = transferPool;
    transferQueue.immCmdBuf = cmdBufs[0];
    transferQueue.eofCmdBuf = cmdBufs[1];
    for (const auto &queueInfo : deviceInfo.queues)
    {
        if (deviceInfo.transferQueueFamily == queueInfo.queueFamilyIdx)
        {
            transferQueue.queue = queueInfo.queue;
        }
    }
    return transferQueue;
}

void transferImmediate(const TransferQueue &queue, Uploadable upload, VkBufferCopy copyInfo)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(queue.immCmdBuf, &commandBufferBeginInfo);
    vkCmdCopyBuffer(queue.immCmdBuf, upload.bufferPair.first, upload.bufferPair.second, 1, &copyInfo);
    vkEndCommandBuffer(queue.immCmdBuf);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &queue.immCmdBuf;

    vkQueueSubmit(queue.queue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(queue.queue);
}

void transferEndOfFrame(TransferQueue &queue)
{
}

int main()
{
    std::filesystem::path selfPath = std::filesystem::read_symlink("/proc/self/exe");
    std::filesystem::path selfDir = selfPath.parent_path();

    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Could not init glfw" << std::endl;
        return -1;
    }
    VkInstance instance = createInstance();
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT messenger = createDebugMessenger(instance);
#endif

    PhysDeviceInfo physDeviceInfo = getPhysicalDevice(instance);
    WindowInfo windowInfo = createWindow(instance, physDeviceInfo);
    DeviceInfo deviceInfo = createDevice(instance, physDeviceInfo, windowInfo);
    VmaAllocator allocator = createVmaAllocator(instance, physDeviceInfo.device, deviceInfo.device);
    SwapchainInfo swapchainInfo = createSwapchain(deviceInfo, windowInfo);
    VkPipelineLayout pipelineLayout = createPipelineLayout(deviceInfo);
    VkRenderPass renderPass = createRenderPass(deviceInfo, windowInfo);
    ImageGroupInfo swapChainImages = getSwapChainImages(deviceInfo, windowInfo, swapchainInfo, renderPass);
    RenderContext renderContext = createRenderContext(deviceInfo, swapchainInfo);

    TransferQueue transferQueue = createTransferQueue(deviceInfo);

    std::string vertShaderPath = selfDir.string() + "/shaders/triangle.vert.spv";
    std::string fragShaderPath = selfDir.string() + "/shaders/triangle.frag.spv";
    std::string suzannePath = selfDir.string() + "/resources/suzanne.obj";

    VkShaderModule vertexShader = loadShaderModule(deviceInfo.device, vertShaderPath.c_str());
    VkShaderModule fragmentShader = loadShaderModule(deviceInfo.device, fragShaderPath.c_str());
    Mesh suzanneMesh = loadMesh(suzannePath, transferQueue, allocator, deviceInfo);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {};
    shaders.push_back(getShaderStageCreateInfo(vertexShader, VK_SHADER_STAGE_VERTEX_BIT));
    shaders.push_back(getShaderStageCreateInfo(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT));

    VkPipeline pipeline = createPipeline(deviceInfo.device, renderPass, pipelineLayout, shaders);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!glfwWindowShouldClose(windowInfo.window))
    {
        // int minimized = glfwGetWindowAttrib(windowInfo.window, GLFW);

        VkResult drawStatus = drawFrame(deviceInfo, swapchainInfo, renderContext, swapChainImages, renderPass, nbFrames, pipeline, windowInfo);
        if (drawStatus == VK_ERROR_OUT_OF_DATE_KHR || drawStatus == VK_SUBOPTIMAL_KHR)
        {
            auto newSwapchainInfo = createSwapchain(deviceInfo, windowInfo, swapchainInfo.swapchain);
            auto newSwapChainImages = getSwapChainImages(deviceInfo, windowInfo, newSwapchainInfo, renderPass);
            vkDeviceWaitIdle(deviceInfo.device);
            freeSwapchainImages(deviceInfo.device, swapChainImages);
            vkDestroySwapchainKHR(deviceInfo.device, swapchainInfo.swapchain, nullptr);
            swapchainInfo = newSwapchainInfo;
            swapChainImages = newSwapChainImages;
        }
        else
        {
            VK_LOG_ERR(drawStatus);
        }

        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0)
        {
            printf("%f ms/frame\n", 1000.0 / double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }

        glfwPollEvents();
    }

#ifndef NDEBUG
    destroyDebugMessenger(instance, messenger);
#endif
    // wait for device to idle before destroying vulkan objects
    vkDeviceWaitIdle(deviceInfo.device);

    destroyRenderContext(deviceInfo.device, renderContext);
    vkDestroyShaderModule(deviceInfo.device, vertexShader, nullptr);
    vkDestroyShaderModule(deviceInfo.device, fragmentShader, nullptr);
    vkDestroyPipeline(deviceInfo.device, pipeline, nullptr);
    freeSwapchainImages(deviceInfo.device, swapChainImages);
    vkDestroyRenderPass(deviceInfo.device, renderPass, nullptr);
    vkDestroyPipelineLayout(deviceInfo.device, pipelineLayout, nullptr);
    vkDestroySwapchainKHR(deviceInfo.device, swapchainInfo.swapchain, nullptr);
    vkDestroySurfaceKHR(instance, windowInfo.surface, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(deviceInfo.device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
    return 0;
}