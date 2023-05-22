#include "common.h"
#include "renderer.h"
#include "types.h"
#include <array>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <span>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define GGLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
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

SwapchainInfo Renderer::createSwapchain()
{
    int width;
    int height;
    glfwGetWindowSize(m_windowInfo.window, &width, &height);

    glm::clamp((uint32_t)width, m_windowInfo.surfaceCapabilities.minImageExtent.width,
               m_windowInfo.surfaceCapabilities.maxImageExtent.width);
    glm::clamp((uint32_t)height, m_windowInfo.surfaceCapabilities.minImageExtent.height,
               m_windowInfo.surfaceCapabilities.maxImageExtent.height);

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainCreateInfo.oldSwapchain = m_swapchainInfo.swapchain;
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

VkRenderPass Renderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.format = m_windowInfo.preferredSurfaceFormat.format;
    colorAttachment.samples = m_numSamples;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = m_numSamples;

    VkAttachmentDescription resolveAttachment = {};
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    resolveAttachment.format = m_windowInfo.preferredSurfaceFormat.format;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment, resolveAttachment};

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentReference = {};
    resolveAttachmentReference.attachment = 2;
    resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.pResolveAttachments = &resolveAttachmentReference;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkSubpassDependency colorDependency = {};
    colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    colorDependency.dstSubpass = 0;
    colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.srcAccessMask = 0;
    colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[] = {colorDependency, depthDependency};

    VkRenderPassCreateInfo renderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassCreateInfo.attachmentCount = ARR_CNT(attachments);
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = ARR_CNT(dependencies);
    renderPassCreateInfo.pDependencies = dependencies;

    VkRenderPass renderPass = nullptr;
    VK_LOG_ERR(vkCreateRenderPass(m_deviceInfo.device, &renderPassCreateInfo, nullptr, &renderPass));
    return renderPass;
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

std::vector<DepthBuffer> Renderer::createDepthBuffers()
{
    VkImageCreateInfo depthBufferInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depthBufferInfo.imageType = VK_IMAGE_TYPE_2D;
    depthBufferInfo.mipLevels = 1;
    depthBufferInfo.arrayLayers = 1;
    depthBufferInfo.extent = {m_swapchainInfo.width, m_swapchainInfo.height, 1};
    depthBufferInfo.format = VK_FORMAT_D32_SFLOAT;
    depthBufferInfo.samples = m_numSamples;
    depthBufferInfo.queueFamilyIndexCount = 1;
    depthBufferInfo.pQueueFamilyIndices = &m_deviceInfo.graphicsQueueFamily;
    depthBufferInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthBufferInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthBufferInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    std::vector<DepthBuffer> depthBuffers;
    for (unsigned i = 0; i < m_swapchainInfo.numImages; i++)
    {
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        VkImage depthBuf;
        VkImageView depthView;

        VK_LOG_ERR(vmaCreateImage(m_vmaAllocator, &depthBufferInfo, &allocCreateInfo, &depthBuf, &allocation, &allocationInfo));
        imageViewInfo.image = depthBuf;
        VK_LOG_ERR(vkCreateImageView(m_deviceInfo.device, &imageViewInfo, nullptr, &depthView));

        DepthBuffer depthBuffer = {};
        depthBuffer.depthBuf = depthBuf;
        depthBuffer.imageAlloc = allocation;
        depthBuffer.depthView = depthView;

        depthBuffers.push_back(depthBuffer);
    }

    return depthBuffers;
}

VkDescriptorSetLayout Renderer::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding diffuseTextureBinding = {};
    diffuseTextureBinding.binding = 0;
    diffuseTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseTextureBinding.descriptorCount = 1;
    diffuseTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding aoTextureBinding = {};
    aoTextureBinding.binding = 1;
    aoTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    aoTextureBinding.descriptorCount = 1;
    aoTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding emissiveTextureBinding = {};
    emissiveTextureBinding.binding = 2;
    emissiveTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    emissiveTextureBinding.descriptorCount = 1;
    emissiveTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normalMapBinding = {};
    normalMapBinding.binding = 3;
    normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalMapBinding.descriptorCount = 1;
    normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {diffuseTextureBinding, aoTextureBinding, emissiveTextureBinding, normalMapBinding};

    VkDescriptorSetLayoutCreateInfo perMeshDescriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    perMeshDescriptorSetLayoutCreateInfo.flags = 0;
    perMeshDescriptorSetLayoutCreateInfo.bindingCount = ARR_CNT(bindings);
    perMeshDescriptorSetLayoutCreateInfo.pBindings = bindings;

    VkDescriptorSetLayout perMeshDescriptorSetLayout;
    VK_LOG_ERR(
        vkCreateDescriptorSetLayout(m_deviceInfo.device, &perMeshDescriptorSetLayoutCreateInfo, nullptr, &perMeshDescriptorSetLayout));
    return perMeshDescriptorSetLayout;
}

VkPipelineLayout Renderer::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(PushConstants);
    range.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPushConstantRange ranges[] = {range};

    pipelineLayoutCreateInfo.pushConstantRangeCount = ARR_CNT(ranges);
    pipelineLayoutCreateInfo.pPushConstantRanges = ranges;

    VkPipelineLayout pipelineLayout = nullptr;
    VK_LOG_ERR(vkCreatePipelineLayout(m_deviceInfo.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
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

ImageGroupInfo Renderer::getSwapchainImages()
{
    ImageGroupInfo swapchainImages = {};

    uint32_t swapchainImageCount = 0;
    VK_LOG_ERR(vkGetSwapchainImagesKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, &swapchainImageCount, nullptr));
    swapchainImages.msaaImages = std::vector<VkImage>(swapchainImageCount);
    swapchainImages.msaaImageAllocations = std::vector<VmaAllocation>(swapchainImageCount);
    swapchainImages.msaaImageViews = std::vector<VkImageView>(swapchainImageCount);
    swapchainImages.images = std::vector<VkImage>(swapchainImageCount);
    swapchainImages.imageViews = std::vector<VkImageView>(swapchainImageCount);
    swapchainImages.frameBuffers = std::vector<VkFramebuffer>(swapchainImageCount);
    VK_LOG_ERR(
        vkGetSwapchainImagesKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, &swapchainImageCount, swapchainImages.images.data()));

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
        imageViewCreateInfo.format = m_windowInfo.preferredSurfaceFormat.format;
        imageViewCreateInfo.subresourceRange = subResourceRange;

        VK_LOG_ERR(vkCreateImageView(m_deviceInfo.device, &imageViewCreateInfo, nullptr, &swapchainImages.imageViews[i]));

        VkImageCreateInfo msaaImageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        msaaImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        msaaImageCreateInfo.mipLevels = 1;
        msaaImageCreateInfo.arrayLayers = 1;
        msaaImageCreateInfo.extent = {m_swapchainInfo.width, m_swapchainInfo.height, 1};
        msaaImageCreateInfo.format = m_windowInfo.preferredSurfaceFormat.format;
        msaaImageCreateInfo.samples = m_numSamples;
        msaaImageCreateInfo.queueFamilyIndexCount = 1;
        msaaImageCreateInfo.pQueueFamilyIndices = &m_deviceInfo.graphicsQueueFamily;
        msaaImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        msaaImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        msaaImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        msaaImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VmaAllocationInfo allocationInfo;
        VK_LOG_ERR(vmaCreateImage(m_vmaAllocator, &msaaImageCreateInfo, &allocationCreateInfo, &swapchainImages.msaaImages[i],
                                  &swapchainImages.msaaImageAllocations[i], &allocationInfo));

        VkImageViewCreateInfo msaaImageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        msaaImageViewCreateInfo.image = swapchainImages.msaaImages[i];
        msaaImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        msaaImageViewCreateInfo.format = m_windowInfo.preferredSurfaceFormat.format;
        msaaImageViewCreateInfo.subresourceRange = subResourceRange;

        VK_LOG_ERR(vkCreateImageView(m_deviceInfo.device, &msaaImageViewCreateInfo, nullptr, &swapchainImages.msaaImageViews[i]));
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageView attachments[] = {swapchainImages.msaaImageViews[i], m_depthBuffers[i].depthView, swapchainImages.imageViews[i]};

        VkFramebufferCreateInfo framebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferCreateInfo.renderPass = m_renderPass;
        framebufferCreateInfo.attachmentCount = ARR_CNT(attachments);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = m_swapchainInfo.width;
        framebufferCreateInfo.height = m_swapchainInfo.height;
        framebufferCreateInfo.layers = 1;

        VK_LOG_ERR(vkCreateFramebuffer(m_deviceInfo.device, &framebufferCreateInfo, nullptr, &swapchainImages.frameBuffers[i]));
    }

    return swapchainImages;
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

VkResult Renderer::draw(Mesh &mesh, VkPipeline pipeline)
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    {
        glm::vec3 camPos = {0.f, -1.f, -4.f};

        view = glm::translate(glm::mat4(1.f), camPos);
        // camera projection
        projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 1.0f, 5000.0f);
        projection[1][1] *= -1;
        // model rotation
        model = glm::rotate(glm::mat4{1.0f}, glm::radians(m_frameCount * 0.005f), glm::vec3(0, 1, 0))
              * glm::rotate(glm::mat4{1.0f}, glm::radians(90.0f), glm::vec3(1, 0, 0));
    }
    PushConstants pushConstants = {};
    pushConstants.M = model;
    pushConstants.VP = projection * view;

    uint32_t frameIdx = m_frameCount % m_swapchainInfo.numImages;
    m_frameCount++;

    VK_LOG_ERR(vkWaitForFences(m_deviceInfo.device, 1, &m_renderContext.fences[frameIdx], VK_TRUE, UINT64_MAX));
    VK_LOG_ERR(vkResetFences(m_deviceInfo.device, 1, &m_renderContext.fences[frameIdx]));

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = {m_swapchainInfo.width, m_swapchainInfo.height};
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_renderPass;

    VkClearValue clearValues[3];
    clearValues[0].color = {0.f, 0.f, 0.f, 0.f};
    clearValues[1].depthStencil = {1.f, 0};
    clearValues[2].color = {0.f, 0.f, 0.f, 0.f};

    renderPassBeginInfo.clearValueCount = 3;
    renderPassBeginInfo.pClearValues = clearValues;

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(m_windowInfo.window, &windowWidth, &windowHeight);

    VkViewport viewport;
    viewport.height = static_cast<float>(windowHeight);
    viewport.width = static_cast<float>(windowWidth);
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)windowWidth, (uint32_t)windowHeight};

    renderPassBeginInfo.framebuffer = m_swapchainImages.frameBuffers[frameIdx];

    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VK_LOG_ERR(vkBeginCommandBuffer(m_renderContext.commandBuffers[frameIdx], &commandBufferBeginInfo));
    vkCmdSetViewport(m_renderContext.commandBuffers[frameIdx], 0, 1, &viewport);
    vkCmdSetScissor(m_renderContext.commandBuffers[frameIdx], 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(m_renderContext.commandBuffers[frameIdx], &renderPassBeginInfo, subpassContents);
    vkCmdBindPipeline(m_renderContext.commandBuffers[frameIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdPushConstants(m_renderContext.commandBuffers[frameIdx], m_pipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(PushConstants),
                       &pushConstants);

    mesh.drawMesh(m_renderContext.commandBuffers[frameIdx], m_pipelineLayout);

    vkCmdEndRenderPass(m_renderContext.commandBuffers[frameIdx]);
    VK_LOG_ERR(vkEndCommandBuffer(m_renderContext.commandBuffers[frameIdx]));

    uint32_t imageIndex = 0;
    VK_LOG_ERR(vkAcquireNextImageKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, UINT64_MAX, m_renderContext.imgAvailableSem[frameIdx],
                                     nullptr, &imageIndex));

    VkPipelineStageFlags waitDstStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_renderContext.imgAvailableSem[frameIdx];
    submitInfo.pWaitDstStageMask = &waitDstStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_renderContext.commandBuffers[frameIdx];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderContext.renderDoneSem[frameIdx];

    VK_LOG_ERR(
        vkQueueSubmit(m_deviceInfo.queues[m_deviceInfo.graphicsQueueFamily].queue, 1, &submitInfo, m_renderContext.fences[frameIdx]));

    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderContext.renderDoneSem[frameIdx];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchainInfo.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(m_deviceInfo.queues[m_deviceInfo.presentQueueFamily].queue, &presentInfo);
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

void Renderer::freeSwapchainImages()
{
    for (auto &fb : m_swapchainImages.frameBuffers)
    {
        vkDestroyFramebuffer(m_deviceInfo.device, fb, nullptr);
    }
    for (auto &iv : m_swapchainImages.imageViews)
    {
        vkDestroyImageView(m_deviceInfo.device, iv, nullptr);
    }
    for (auto &iv : m_swapchainImages.msaaImageViews)
    {
        vkDestroyImageView(m_deviceInfo.device, iv, nullptr);
    }
    for (uint32_t i = 0; i < m_swapchainImages.msaaImages.size(); i++)
    {
        vmaDestroyImage(m_vmaAllocator, m_swapchainImages.msaaImages[i], m_swapchainImages.msaaImageAllocations[i]);
    }
}

void Renderer::destroyDepthBuffers()
{
    for (auto &buffer : m_depthBuffers)
    {
        vkDestroyImageView(m_deviceInfo.device, buffer.depthView, nullptr);
        vmaDestroyImage(m_vmaAllocator, buffer.depthBuf, buffer.imageAlloc);
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

void Renderer::resize()
{
    m_frameCount = 0;
    std::vector<VkFence> &fences = m_renderContext.fences;
    vkWaitForFences(m_deviceInfo.device, fences.size(), fences.data(), VK_TRUE, UINT64_MAX);

    SwapchainInfo swapchainInfo = createSwapchain();
    vkDestroySwapchainKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, nullptr);
    m_swapchainInfo = swapchainInfo;

    freeSwapchainImages();
    destroyDepthBuffers();

    m_depthBuffers = createDepthBuffers();
    m_swapchainImages = getSwapchainImages();
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
    return dst;
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
    , m_depthBuffers(createDepthBuffers())
    , m_descriptorSetLayout(createDescriptorSetLayout())
    , m_pipelineLayout(createPipelineLayout())
    , m_descriptorPools({createDescriptorPool()})
    , m_renderPass(createRenderPass())
    , m_swapchainImages(getSwapchainImages())
    , m_renderContext(createRenderContext())
    , m_transferQueue(createTransferQueue())
{
}

Renderer::~Renderer()
{
#ifndef NDEBUG
    destroyDebugMessenger();
#endif
    destroyTransferQueue();
    destroyRenderContext();
    destroyDescriptorPools();
    vkDestroyDescriptorSetLayout(m_deviceInfo.device, m_descriptorSetLayout, nullptr);
    freeSwapchainImages();
    vkDestroyRenderPass(m_deviceInfo.device, m_renderPass, nullptr);
    vkDestroyPipelineLayout(m_deviceInfo.device, m_pipelineLayout, nullptr);
    destroyDepthBuffers();
    vkDestroySwapchainKHR(m_deviceInfo.device, m_swapchainInfo.swapchain, nullptr);
    vkDestroySurfaceKHR(m_instance, m_windowInfo.surface, nullptr);
    vmaDestroyAllocator(m_vmaAllocator);
    vkDestroyDevice(m_deviceInfo.device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
    glfwTerminate();
}
