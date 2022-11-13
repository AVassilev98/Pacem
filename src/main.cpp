#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "array"
#include "common.h"
#include "iostream"
#include "vector"
#include <cstring>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#ifndef NDEBUG
#define VK_LOG_ERR(f_)                                                                                                                                         \
    {                                                                                                                                                          \
        VkResult retCode = f_;                                                                                                                                 \
        if (retCode != VK_SUCCESS)                                                                                                                             \
        {                                                                                                                                                      \
            std::cerr << string_VkResult(retCode) << " Returned from " << __FUNCTION__ << " in " << __FILE__ << ":" << __LINE__ << std::endl;                  \
        }                                                                                                                                                      \
    }
#else
#define VK_LOG_ERR(f_) f_
#endif

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

struct WindowInfo
{
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;

    VkSurfaceFormatKHR preferredSurfaceFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
};

struct PhysDeviceInfo
{
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    std::vector<VkQueueFamilyProperties> queueProperties;
};

struct QueueInfo
{
    VkQueue queue;
    VkQueueFamilyProperties queueProperties;
    uint32_t queueFamilyIdx;
    uint32_t queueIdx;
};

struct DeviceInfo
{
    VkDevice device;
    std::vector<QueueInfo> queues;
};

struct SwapchainInfo
{
    VkSwapchainKHR swapchain;
    uint32_t numImages;
};

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

static DeviceInfo createDevice(VkInstance instance, const PhysDeviceInfo &physDeviceInfo)
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
    VK_LOG_ERR(vkCreateSwapchainKHR(deviceInfo.device, &swapchainCreateInfo, nullptr, &swapchainInfo.swapchain));

    return swapchainInfo;
}

static VkRenderPass createRenderPass(VkDevice device)
{
    if (!device)
    {
        return nullptr;
    }

    VkRenderPassCreateInfo renderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

    VkAttachmentDescription attachmentDescription;
    // attachmentDescription.flags

    // VkRenderPass renderPass = nullptr;
    // vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass)

    return nullptr;
}

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Could not init glfw" << std::endl;
        return -1;
    }
    VkInstance instance = createInstance();
    PhysDeviceInfo physDeviceInfo = getPhysicalDevice(instance);
    WindowInfo windowInfo = createWindow(instance, physDeviceInfo);
    DeviceInfo deviceInfo = createDevice(instance, physDeviceInfo);

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT messenger = createDebugMessenger(instance);
#endif

    SwapchainInfo swapchainInfo = createSwapchain(deviceInfo, windowInfo);

    while (!glfwWindowShouldClose(windowInfo.window))
    {
        glfwPollEvents();
    }

#ifndef NDEBUG
    destroyDebugMessenger(instance, messenger);
#endif

    vkDestroySwapchainKHR(deviceInfo.device, swapchainInfo.swapchain, nullptr);
    vkDestroySurfaceKHR(instance, windowInfo.surface, nullptr);
    vkDestroyDevice(deviceInfo.device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
    return 0;
}