#include <array>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <span>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define GGLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "common.h"
#include "renderer.h"
#include "renderpass.h"
#include "types.h"

[[nodiscard]] Renderer &Renderer::Get()
{
    static Renderer device;
    return device;
}

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
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = message;

    return messengerCreateInfo;
}

VkInstance Renderer::createInstance()
{
    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

    uint32_t layerCount = 0;
    VK_LOG_ERR(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
    printf("%u\n", layerCount);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK_LOG_ERR(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const auto &layer : availableLayers)
    {
        uint32_t extensionCount{};
        VK_LOG_ERR(vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        VK_LOG_ERR(vkEnumerateInstanceExtensionProperties(layer.layerName, &extensionCount, availableExtensions.data()));
        std::cout << layer.layerName << "\n";
        for (const auto &extension : availableExtensions)
        {
            std::cout << "\t" << extension.extensionName << "\n";
        }
        std::cout << std::endl;
    }

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Para-ray";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t requiredExtensionCount = 0;
    const char **requiredExtensions_cstr = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    std::vector<const char *> requiredExtensions
        = std::vector<const char *>(requiredExtensions_cstr, requiredExtensions_cstr + requiredExtensionCount);

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
    VK_LOG_ERR_FATAL(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
    return instance;
}

VkDebugUtilsMessengerEXT Renderer::createDebugMessenger()
{
    using MessengerFuncPtr = PFN_vkCreateDebugUtilsMessengerEXT;
    MessengerFuncPtr createDebugMessagerFunc = (MessengerFuncPtr)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (!createDebugMessagerFunc)
    {
        std::cerr << "Could not find proc addr vkCreateDebugUtilsMessengerEXT" << std::endl;
        return nullptr;
    }

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = getMessengerCreateInfo();
    VkDebugUtilsMessengerEXT messenger = nullptr;
    VK_LOG_ERR(createDebugMessagerFunc(m_instance, &messengerCreateInfo, nullptr, &messenger));
    return messenger;
}

const PhysDeviceInfo Renderer::createPhysicalDeviceInfo()
{
    uint32_t numPhysicalDevices = 0;
    VK_LOG_ERR(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    VK_LOG_ERR(vkEnumeratePhysicalDevices(m_instance, &numPhysicalDevices, physicalDevices.data()));

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

const WindowInfo Renderer::createWindow()
{
    WindowInfo windowInfo = {};

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
    windowInfo.window = glfwCreateWindow(1920, 1080, "Pararay", nullptr, nullptr);

    VK_LOG_ERR(glfwCreateWindowSurface(m_instance, windowInfo.window, nullptr, &windowInfo.surface));

    uint32_t surfaceFormatCount = 0;
    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDeviceInfo.device, windowInfo.surface, &surfaceFormatCount, nullptr));
    windowInfo.surfaceFormats = std::vector<VkSurfaceFormatKHR>(surfaceFormatCount);
    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physDeviceInfo.device, windowInfo.surface, &surfaceFormatCount,
                                                    windowInfo.surfaceFormats.data()));

    bool foundPreferredSurfaceFormat = false;
    for (auto &surfaceFormat : windowInfo.surfaceFormats)
    {
        if (surfaceFormat.format == windowInfo.preferredSurfaceFormat.format
            && surfaceFormat.colorSpace == windowInfo.preferredSurfaceFormat.colorSpace)
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
    VK_LOG_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physDeviceInfo.device, windowInfo.surface, &presentModeCount, nullptr));
    windowInfo.presentModes = std::vector<VkPresentModeKHR>(presentModeCount);
    VK_LOG_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physDeviceInfo.device, windowInfo.surface, &presentModeCount,
                                                         windowInfo.presentModes.data()));

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

    VK_LOG_ERR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDeviceInfo.device, windowInfo.surface, &windowInfo.surfaceCapabilities));

    return windowInfo;
}

const DeviceInfo Renderer::createDevice()
{
    uint32_t layerCount = 0;
    VK_LOG_ERR(vkEnumerateDeviceLayerProperties(m_physDeviceInfo.device, &layerCount, nullptr));
    printf("%u\n", layerCount);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    VK_LOG_ERR(vkEnumerateDeviceLayerProperties(m_physDeviceInfo.device, &layerCount, availableLayers.data()));

    for (const auto &layer : availableLayers)
    {
        uint32_t extensionCount{};
        VK_LOG_ERR(vkEnumerateDeviceExtensionProperties(m_physDeviceInfo.device, layer.layerName, &extensionCount, nullptr));
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        VK_LOG_ERR(
            vkEnumerateDeviceExtensionProperties(m_physDeviceInfo.device, layer.layerName, &extensionCount, availableExtensions.data()));
        std::cout << layer.layerName << "\n";
        for (const auto &extension : availableExtensions)
        {
            std::cout << "\t" << extension.extensionName << "\n";
        }
        std::cout << std::endl;
    }
    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_physDeviceInfo.queueProperties.size());
    constexpr float queuePriority = 1.0f;
    for (uint32_t i = 0; i < queueCreateInfos.size(); i++)
    {
        auto &createInfo = queueCreateInfos[i];
        const auto &queueProperties = m_physDeviceInfo.queueProperties[i];

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueCount = 1;
        createInfo.queueFamilyIndex = i;
        createInfo.pQueuePriorities = &queuePriority;
    }

    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &m_physDeviceInfo.deviceFeatures;

    std::array<const char *, 1> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceCreateInfo.enabledExtensionCount = requiredExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    DeviceInfo deviceInfo = {};
    VK_LOG_ERR(vkCreateDevice(m_physDeviceInfo.device, &deviceCreateInfo, nullptr, &deviceInfo.device));

    deviceInfo.queues.resize(m_physDeviceInfo.queueProperties.size());
    for (uint32_t i = 0; i < m_physDeviceInfo.queueProperties.size(); i++)
    {
        deviceInfo.queues[i].queueFamilyIdx = i;
        deviceInfo.queues[i].queueIdx = 0;
        deviceInfo.queues[i].queueProperties = m_physDeviceInfo.queueProperties[i];
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
        if (deviceInfo.queues[i].queueProperties.queueFlags & VK_QUEUE_TRANSFER_BIT
            && !(deviceInfo.queues[i].queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            deviceInfo.transferQueueFamily = i;
            break;
        }
    }

    for (uint32_t i = 0; i < deviceInfo.queues.size(); i++)
    {
        VkBool32 presentSupport = VK_FALSE;
        VK_LOG_ERR(vkGetPhysicalDeviceSurfaceSupportKHR(m_physDeviceInfo.device, i, m_windowInfo.surface, &presentSupport));

        if (presentSupport == VK_TRUE)
        {
            deviceInfo.presentQueueFamily = i;
            break;
        }
    }

    return deviceInfo;
}

SwapchainInfo Renderer::createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE)
{
    int width;
    int height;
    glfwGetWindowSize(m_windowInfo.window, &width, &height);

    glm::clamp((uint32_t)width, m_windowInfo.surfaceCapabilities.minImageExtent.width,
               m_windowInfo.surfaceCapabilities.maxImageExtent.width);
    glm::clamp((uint32_t)height, m_windowInfo.surfaceCapabilities.minImageExtent.height,
               m_windowInfo.surfaceCapabilities.maxImageExtent.height);

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.oldSwapchain = oldSwapchain;
    swapchainCreateInfo.imageFormat = m_windowInfo.preferredSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = m_windowInfo.preferredSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.minImageCount = numPreferredImages > m_windowInfo.surfaceCapabilities.maxImageCount
                                          ? m_windowInfo.surfaceCapabilities.maxImageCount
                                          : numPreferredImages;
    swapchainCreateInfo.presentMode = m_windowInfo.preferredPresentMode;
    swapchainCreateInfo.surface = m_windowInfo.surface;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    std::vector<uint32_t> queueFamilyIndices;
    for (const auto &queueInfo : m_deviceInfo.queues)
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
    VK_LOG_ERR(vkCreateSwapchainKHR(m_deviceInfo.device, &swapchainCreateInfo, nullptr, &swapchainInfo.swapchain));

    return swapchainInfo;
}

VkSampleCountFlagBits Renderer::getMaxSamples()
{
    VkSampleCountFlags counts = m_physDeviceInfo.deviceProperties.limits.framebufferColorSampleCounts
                              & m_physDeviceInfo.deviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        std::cout << "Max sample count is 64xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_64_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        std::cout << "Max sample count is 32xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_32_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        std::cout << "Max sample count is 16xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_16_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        std::cout << "Max sample count is 8xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_8_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        std::cout << "Max sample count is 4xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_4_BIT;
    }
    else if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        std::cout << "Max sample count is 2xMSAA" << std::endl;
        return VK_SAMPLE_COUNT_2_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

const VmaAllocator Renderer::createVmaAllocator()
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorCreateInfo.physicalDevice = m_physDeviceInfo.device;
    allocatorCreateInfo.device = m_deviceInfo.device;
    allocatorCreateInfo.instance = m_instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    VK_LOG_ERR(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
    return allocator;
}

VkDescriptorPool Renderer::createDescriptorPool()
{
    static const std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 500},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 500},
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolCreateInfo.maxSets = 1000;
    descriptorPoolCreateInfo.poolSizeCount = sizes.size();
    descriptorPoolCreateInfo.pPoolSizes = sizes.data();
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorPool;
    VK_LOG_ERR(vkCreateDescriptorPool(m_deviceInfo.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
}

VkDescriptorSet Renderer::allocateDescriptorSet(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPools.back(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(m_deviceInfo.device, &descriptorSetLayoutCreateInfo, &descriptorSet);

    bool poolFull = false;
    switch (result)
    {
    case VK_SUCCESS:
        return descriptorSet;
    case VK_ERROR_FRAGMENTED_POOL:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        poolFull = true;
        break;
    default:
        VK_LOG_ERR(result);
        return VK_NULL_HANDLE;
    }

    if (poolFull)
    {
        std::cout << "Reallocating Descriptor Pool" << std::endl;
        m_descriptorPools.push_back(createDescriptorPool());
        descriptorSetLayoutCreateInfo.descriptorPool = m_descriptorPools.back();
        VK_LOG_ERR(vkAllocateDescriptorSets(m_deviceInfo.device, &descriptorSetLayoutCreateInfo, &descriptorSet);)
    };
    return descriptorSet;
}

void Renderer::destroyDescriptorPools()
{
    for (VkDescriptorPool &descriptorPool : m_descriptorPools)
    {
        vkDestroyDescriptorPool(m_deviceInfo.device, descriptorPool, nullptr);
    }
}

std::vector<Image> Renderer::getSwapchainImages()
{
    std::vector<Image> images(getMaxNumFramesInFlight());
    std::vector<VkImage> vkImages(getMaxNumFramesInFlight());

    uint32_t swapchainImageCount = 0;
    VK_LOG_ERR(vkGetSwapchainImagesKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, &swapchainImageCount, nullptr));
    VK_LOG_ERR(vkGetSwapchainImagesKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, &swapchainImageCount, vkImages.data()));

    VkImageSubresourceRange subResourceRange;
    subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceRange.baseArrayLayer = 0;
    subResourceRange.baseMipLevel = 0;
    subResourceRange.layerCount = 1;
    subResourceRange.levelCount = 1;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        images[i].m_image = vkImages[i];
        images[i].m_width = m_swapchainInfo.width;
        images[i].m_height = m_swapchainInfo.height;

        VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.image = vkImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_windowInfo.preferredSurfaceFormat.format;
        imageViewCreateInfo.subresourceRange = subResourceRange;

        VK_LOG_ERR(vkCreateImageView(m_deviceInfo.device, &imageViewCreateInfo, nullptr, &images[i].m_imageView));
    }

    return images;
}

void Renderer::freeSwapchainImages()
{
    for (Image &image : m_swapchainImages)
    {
        vkDestroyImageView(m_deviceInfo.device, image.m_imageView, nullptr);
    }
}

static VkCommandPool createCommandPool(const VkDevice device, uint32_t queueFamilyIndex)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool commandPool = nullptr;
    VK_LOG_ERR(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
    return commandPool;
}

static VkCommandBuffer createCommandBuffer(const DeviceInfo &deviceInfo, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer commandBuffer = nullptr;
    VK_LOG_ERR(vkAllocateCommandBuffers(deviceInfo.device, &commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
}

RenderContext Renderer::createRenderContext()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    RenderContext renderContext = {};
    renderContext.commandPools.resize(m_swapchainInfo.numImages);
    renderContext.commandBuffers.resize(m_swapchainInfo.numImages);
    renderContext.imgAvailableSem.resize(m_swapchainInfo.numImages);
    renderContext.renderDoneSem.resize(m_swapchainInfo.numImages);
    renderContext.fences.resize(m_swapchainInfo.numImages);

    for (uint32_t i = 0; i < m_swapchainInfo.numImages; i++)
    {
        renderContext.commandPools[i] = createCommandPool(m_deviceInfo.device, m_deviceInfo.graphicsQueueFamily);
        renderContext.commandBuffers[i] = createCommandBuffer(m_deviceInfo, renderContext.commandPools[i]);
        VK_LOG_ERR(vkCreateSemaphore(m_deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.imgAvailableSem[i]));
        VK_LOG_ERR(vkCreateSemaphore(m_deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.renderDoneSem[i]));
        VK_LOG_ERR(vkCreateFence(m_deviceInfo.device, &fenceCreateInfo, nullptr, &renderContext.fences[i]));
    }

    return renderContext;
}

TransferQueue Renderer::createTransferQueue()
{
    TransferQueue transferQueue = {};

    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = m_deviceInfo.transferQueueFamily;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool transferPool = createCommandPool(m_deviceInfo.device, m_deviceInfo.transferQueueFamily);
    VkCommandPool graphicsPool = createCommandPool(m_deviceInfo.device, m_deviceInfo.graphicsQueueFamily);

    VkCommandBufferAllocateInfo commandBufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocInfo.commandPool = transferPool;
    commandBufferAllocInfo.commandBufferCount = 2;
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBufferAllocateInfo graphicsCommandBufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    graphicsCommandBufferAllocInfo.commandPool = graphicsPool;
    graphicsCommandBufferAllocInfo.commandBufferCount = 1;
    graphicsCommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer cmdBufs[2] = {};
    VK_LOG_ERR(vkAllocateCommandBuffers(m_deviceInfo.device, &commandBufferAllocInfo, cmdBufs));

    VkCommandBuffer graphicsCmdBuf;
    VK_LOG_ERR(vkAllocateCommandBuffers(m_deviceInfo.device, &graphicsCommandBufferAllocInfo, &graphicsCmdBuf));

    transferQueue.transferCommandPool = transferPool;
    transferQueue.graphicsCommandPool = graphicsPool;
    transferQueue.immCmdBuf = cmdBufs[0];
    transferQueue.eofCmdBuf = cmdBufs[1];
    transferQueue.graphicsCmdBuf = graphicsCmdBuf;
    for (const auto &queueInfo : m_deviceInfo.queues)
    {
        if (m_deviceInfo.transferQueueFamily == queueInfo.queueFamilyIdx)
        {
            transferQueue.transferQueue = queueInfo.queue;
        }

        if (m_deviceInfo.graphicsQueueFamily == queueInfo.queueFamilyIdx)
        {
            transferQueue.graphicsQueue = queueInfo.queue;
        }
    }
    return transferQueue;
}

bool Renderer::exitSignal()
{
    return glfwWindowShouldClose(m_windowInfo.window);
}

VkResult Renderer::draw()
{
    uint32_t frameIdx = (m_frameCount) % m_swapchainInfo.numImages;
    m_frameCount++;
    VK_LOG_ERR(vkWaitForFences(m_deviceInfo.device, 1, &m_renderContext.fences[frameIdx], VK_TRUE, UINT64_MAX));
    VK_LOG_ERR(vkResetFences(m_deviceInfo.device, 1, &m_renderContext.fences[frameIdx]));

    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_LOG_ERR(vkBeginCommandBuffer(m_renderContext.commandBuffers[frameIdx], &commandBufferBeginInfo));

    for (RenderPass *renderPass : m_renderPasses)
    {
        renderPass->draw(m_renderContext.commandBuffers[frameIdx], frameIdx);
    }

    VK_LOG_ERR(vkEndCommandBuffer(m_renderContext.commandBuffers[frameIdx]));

    VkPipelineStageFlags waitDstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_renderContext.imgAvailableSem[frameIdx];
    submitInfo.pWaitDstStageMask = &waitDstStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_renderContext.commandBuffers[frameIdx];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderContext.renderDoneSem[frameIdx];

    uint32_t imageIndex = 0;
    VK_LOG_ERR(vkAcquireNextImageKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, UINT64_MAX, m_renderContext.imgAvailableSem[frameIdx],
                                     nullptr, &imageIndex));
    VK_LOG_ERR(
        vkQueueSubmit(m_deviceInfo.queues[m_deviceInfo.graphicsQueueFamily].queue, 1, &submitInfo, m_renderContext.fences[frameIdx]));

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderContext.renderDoneSem[frameIdx];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchainInfo.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    VkResult presentStatus = vkQueuePresentKHR(m_deviceInfo.queues[m_deviceInfo.presentQueueFamily].queue, &presentInfo);
    if (presentStatus == VK_ERROR_OUT_OF_DATE_KHR || presentStatus == VK_SUBOPTIMAL_KHR)
    {
        vkDeviceWaitIdle(m_deviceInfo.device);
        handleResize();
        return VK_SUCCESS;
    }

    VK_LOG_ERR(presentStatus);
    return presentStatus;
}

void Renderer::wait()
{
    vkDeviceWaitIdle(m_deviceInfo.device);
}

void Renderer::destroyTransferQueue()
{
    vkFreeCommandBuffers(m_deviceInfo.device, m_transferQueue.transferCommandPool, 1, &m_transferQueue.immCmdBuf);
    vkFreeCommandBuffers(m_deviceInfo.device, m_transferQueue.transferCommandPool, 1, &m_transferQueue.eofCmdBuf);
    vkFreeCommandBuffers(m_deviceInfo.device, m_transferQueue.graphicsCommandPool, 1, &m_transferQueue.graphicsCmdBuf);
    vkDestroyCommandPool(m_deviceInfo.device, m_transferQueue.transferCommandPool, nullptr);
    vkDestroyCommandPool(m_deviceInfo.device, m_transferQueue.graphicsCommandPool, nullptr);
}

void Renderer::destroyRenderContext()
{
    for (unsigned i = 0; i < m_renderContext.commandBuffers.size(); i++)
    {
        auto &cmdBuf = m_renderContext.commandBuffers[i];
        vkFreeCommandBuffers(m_deviceInfo.device, m_renderContext.commandPools[i], 1, &cmdBuf);
    }
    for (auto &cmdPool : m_renderContext.commandPools)
    {
        vkDestroyCommandPool(m_deviceInfo.device, cmdPool, nullptr);
    }
    for (auto &sem : m_renderContext.imgAvailableSem)
    {
        vkDestroySemaphore(m_deviceInfo.device, sem, nullptr);
    }
    for (auto &sem : m_renderContext.renderDoneSem)
    {
        vkDestroySemaphore(m_deviceInfo.device, sem, nullptr);
    }
    for (auto &fence : m_renderContext.fences)
    {
        vkDestroyFence(m_deviceInfo.device, fence, nullptr);
    }
}

void Renderer::destroyDebugMessenger()
{
    using MessengerFuncPtr = PFN_vkDestroyDebugUtilsMessengerEXT;
    MessengerFuncPtr destroyDebugMessagerFunc = (MessengerFuncPtr)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!destroyDebugMessagerFunc)
    {
        std::cerr << "Could not find proc addr vkDestroyDebugUtilsMessengerEXT" << std::endl;
        return;
    }

    destroyDebugMessagerFunc(m_instance, m_debugMessenger, nullptr);
}

VkExtent2D Renderer::getWindowExtent()
{
    int width = 0;
    int height = 0;
    glfwGetWindowSize(m_windowInfo.window, &width, &height);

    return VkExtent2D{
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
    };
}

uint32_t Renderer::getQueueFamilyIdx(QueueFamily family)
{
    switch (family)
    {
    case QueueFamily::Graphics:
        return m_deviceInfo.graphicsQueueFamily;
    case QueueFamily::Transfer:
        return m_deviceInfo.transferQueueFamily;
    default:
        assert(false && "Invalid QueueFamily supplied to getQueueFamilyIdx");
        return std::numeric_limits<uint32_t>::max();
    }
}

Buffer Renderer::uploadBufferToGpu(VkBuffer src, Buffer::State &dstState)
{
    VkBufferCopy copyInfo = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = dstState.size,
    };

    Buffer gpuBuf(dstState);
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(m_transferQueue.immCmdBuf, &commandBufferBeginInfo);
    vkCmdCopyBuffer(m_transferQueue.immCmdBuf, src, gpuBuf.m_buffer, 1, &copyInfo);
    vkEndCommandBuffer(m_transferQueue.immCmdBuf);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferQueue.immCmdBuf;

    vkQueueSubmit(m_transferQueue.transferQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(m_transferQueue.transferQueue);

    return gpuBuf;
}

void Renderer::transferImageImmediate(UploadInfo &info)
{
    VkCommandBuffer transferCmdBuffer = createCommandBuffer(m_deviceInfo, m_transferQueue.transferCommandPool);
    VkCommandBuffer graphicsCmdBuffer = createCommandBuffer(m_deviceInfo, m_transferQueue.graphicsCommandPool);

    VkImageSubresourceLayers subResourceLayers = {};
    subResourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceLayers.mipLevel = 0;
    subResourceLayers.baseArrayLayer = 0;
    subResourceLayers.layerCount = 1;

    VkBufferImageCopy bufferImageCopy;
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource = subResourceLayers;
    bufferImageCopy.imageOffset = {0, 0, 0};
    bufferImageCopy.imageExtent = {info.dst.m_width, info.dst.m_height, 1};

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.layerCount = 1;
    subresourceRange.levelCount = 1;

    VkImageMemoryBarrier imageToTransfer = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageToTransfer.srcAccessMask = 0;
    imageToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageToTransfer.srcQueueFamilyIndex = m_deviceInfo.transferQueueFamily;
    imageToTransfer.dstQueueFamilyIndex = m_deviceInfo.transferQueueFamily;
    imageToTransfer.image = info.dst.m_image;
    imageToTransfer.subresourceRange = subresourceRange;

    VkImageMemoryBarrier imageOwnershipTransferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageOwnershipTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageOwnershipTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageOwnershipTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageOwnershipTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageOwnershipTransferBarrier.srcQueueFamilyIndex = m_deviceInfo.transferQueueFamily;
    imageOwnershipTransferBarrier.dstQueueFamilyIndex = m_deviceInfo.graphicsQueueFamily;
    imageOwnershipTransferBarrier.image = info.dst.m_image;
    imageOwnershipTransferBarrier.subresourceRange = subresourceRange;

    VkImageMemoryBarrier imageToReadable = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageToReadable.srcQueueFamilyIndex = m_deviceInfo.graphicsQueueFamily;
    imageToReadable.dstQueueFamilyIndex = m_deviceInfo.graphicsQueueFamily;
    imageToReadable.image = info.dst.m_image;
    imageToReadable.subresourceRange = subresourceRange;

    VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore transferSem;
    vkCreateSemaphore(m_deviceInfo.device, &semCreateInfo, nullptr, &transferSem);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(transferCmdBuffer, &commandBufferBeginInfo);
    vkCmdPipelineBarrier(transferCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageToTransfer);
    vkCmdCopyBufferToImage(transferCmdBuffer, info.src, info.dst.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);

    vkCmdPipelineBarrier(transferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageOwnershipTransferBarrier);
    vkEndCommandBuffer(transferCmdBuffer);

    VkSubmitInfo transferSubmission = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    transferSubmission.commandBufferCount = 1;
    transferSubmission.pCommandBuffers = &transferCmdBuffer;
    transferSubmission.signalSemaphoreCount = 1;
    transferSubmission.pSignalSemaphores = &transferSem;

    vkQueueSubmit(m_transferQueue.transferQueue, 1, &transferSubmission, nullptr);
    vkBeginCommandBuffer(graphicsCmdBuffer, &commandBufferBeginInfo);
    vkCmdPipelineBarrier(graphicsCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageToReadable);
    vkEndCommandBuffer(graphicsCmdBuffer);

    VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    VkSubmitInfo graphicsSubmission = {};
    graphicsSubmission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmission.commandBufferCount = 1;
    graphicsSubmission.pCommandBuffers = &graphicsCmdBuffer;
    graphicsSubmission.pWaitDstStageMask = &waitStageFlags;
    graphicsSubmission.pWaitSemaphores = &transferSem;
    graphicsSubmission.waitSemaphoreCount = 1;

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence transferFence;

    vkCreateFence(m_deviceInfo.device, &fence_create_info, nullptr, &transferFence);
    vkQueueSubmit(m_transferQueue.graphicsQueue, 1, &graphicsSubmission, transferFence);
    vkWaitForFences(m_deviceInfo.device, 1, &transferFence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(m_deviceInfo.device, m_transferQueue.transferCommandPool, 1, &transferCmdBuffer);
    vkFreeCommandBuffers(m_deviceInfo.device, m_transferQueue.graphicsCommandPool, 1, &graphicsCmdBuffer);
    vkDestroySemaphore(m_deviceInfo.device, transferSem, nullptr);
    vkDestroyFence(m_deviceInfo.device, transferFence, nullptr);
}

Image Renderer::uploadImageToGpu(VkBuffer src, Image::State &dstState)
{
    VkImageSubresourceLayers subResourceLayers = {};
    subResourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceLayers.mipLevel = 0;
    subResourceLayers.baseArrayLayer = 0;
    subResourceLayers.layerCount = 1;

    VkBufferImageCopy bufferImageCopy;
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource = subResourceLayers;
    bufferImageCopy.imageOffset = {0, 0, 0};
    bufferImageCopy.imageExtent = {dstState.width, dstState.height, 1};

    Image dst(dstState);
    UploadInfo uploadInfo = {
        .src = src,
        .dst = dst,
    };
    transferImageImmediate(uploadInfo);
    return std::move(dst);
}

void Renderer::updateDescriptor(VkDescriptorSet descriptorSet, const VkDescriptorImageInfo &descriptorImageInfo, uint32_t binding)
{
    VkWriteDescriptorSet writeDiffuseDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDiffuseDescriptorSet.dstSet = descriptorSet;
    writeDiffuseDescriptorSet.dstBinding = binding;
    writeDiffuseDescriptorSet.dstArrayElement = 0;
    writeDiffuseDescriptorSet.descriptorCount = 1;
    writeDiffuseDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDiffuseDescriptorSet.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(m_deviceInfo.device, 1, &writeDiffuseDescriptorSet, 0, nullptr);
}

uint32_t Renderer::getMaxNumFramesInFlight()
{
    return m_swapchainInfo.numImages;
}

const SwapchainInfo &Renderer::getSwapchainInfo()
{
    return m_swapchainInfo;
}

void Renderer::addRenderPass(RenderPass *renderPass)
{
    m_renderPasses.push_back(renderPass);
}

void Renderer::handleResize()
{
    m_frameCount = 0;
    freeSwapchainImages();
    VkSwapchainKHR oldSwapchain = m_swapchainInfo.swapchain;
    m_swapchainInfo = createSwapchain(oldSwapchain);
    vkDestroySwapchainKHR(m_deviceInfo.device, oldSwapchain, nullptr);
    m_swapchainImages = getSwapchainImages();
    for (RenderPass *renderPass : m_renderPasses)
    {
        renderPass->resize(m_swapchainInfo.width, m_swapchainInfo.height);
    }
}

Renderer::Renderer()
    : m_instance(createInstance())
#ifndef NDEBUG
    , m_debugMessenger(createDebugMessenger())
#endif
    , m_physDeviceInfo(createPhysicalDeviceInfo())
    , m_numSamples(getMaxSamples())
    , m_windowInfo(createWindow())
    , m_deviceInfo(createDevice())
    , m_vmaAllocator(createVmaAllocator())
    , m_swapchainInfo(createSwapchain())
    , m_descriptorPools({createDescriptorPool()})
    , m_swapchainImages(getSwapchainImages())
    , m_renderContext(createRenderContext())
    , m_transferQueue(createTransferQueue())
{
}

void Renderer::transferImmediate(std::function<void(VkCommandBuffer cmd)> &&function)
{

    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(m_transferQueue.immCmdBuf, &cmdBufBeginInfo);
    function(m_transferQueue.immCmdBuf);
    vkEndCommandBuffer(m_transferQueue.immCmdBuf);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_transferQueue.immCmdBuf,
    };

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence transferFence;

    vkCreateFence(m_deviceInfo.device, &fence_create_info, nullptr, &transferFence);
    vkQueueSubmit(m_transferQueue.transferQueue, 1, &submitInfo, transferFence);
    vkWaitForFences(m_deviceInfo.device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(m_deviceInfo.device, transferFence, nullptr);
}

void Renderer::graphicsImmediate(std::function<void(VkCommandBuffer cmd)> &&function)
{
    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(m_transferQueue.graphicsCmdBuf, &cmdBufBeginInfo);
    function(m_transferQueue.graphicsCmdBuf);
    vkEndCommandBuffer(m_transferQueue.graphicsCmdBuf);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_transferQueue.graphicsCmdBuf,
    };

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence transferFence;

    vkCreateFence(m_deviceInfo.device, &fence_create_info, nullptr, &transferFence);
    vkQueueSubmit(m_transferQueue.graphicsQueue, 1, &submitInfo, transferFence);
    vkWaitForFences(m_deviceInfo.device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(m_deviceInfo.device, transferFence, nullptr);
}

Renderer::~Renderer()
{
#ifndef NDEBUG
    destroyDebugMessenger();
#endif
    destroyTransferQueue();
    destroyRenderContext();
    destroyDescriptorPools();
    freeSwapchainImages();
    vkDestroySwapchainKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, nullptr);
    vkDestroySurfaceKHR(m_instance, m_windowInfo.surface, nullptr);
    vmaDestroyAllocator(m_vmaAllocator);
    vkDestroyDevice(m_deviceInfo.device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    glfwTerminate();
}
