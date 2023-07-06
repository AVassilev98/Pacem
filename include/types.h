#pragma once
#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "vma.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

template <typename T, typename IdxType>
struct Handle;

template <typename T>
struct PerFrame;

struct Image;
struct Buffer;
struct Framebuffer;

struct RenderPass;

struct WindowInfo
{
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR> presentModes;

    static constexpr VkSurfaceFormatKHR preferredSurfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    static constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
};

struct MVP
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct PhysDeviceInfo
{
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    std::vector<VkQueueFamilyProperties> queueProperties;
};

struct SwapchainInfo
{
    VkSwapchainKHR swapchain;
    uint32_t numImages;
    uint32_t height;
    uint32_t width;
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 textureCoordinate;
};

struct PushConstants
{
    glm::mat4 M;
    glm::mat4 V;
    glm::mat4 P;
};

struct DepthBuffer
{
    VkImage depthBuf;
    VmaAllocation imageAlloc;
    VkImageView depthView;
};

struct UploadableBuffer
{
    VkBuffer src;
    VkBuffer dst;
};

struct UploadableTexture
{
    VkBuffer src;
    VkImage dst;
};

struct TransferQueue
{
    VkQueue transferQueue;
    VkQueue graphicsQueue;
    VkCommandPool transferCommandPool;
    VkCommandPool graphicsCommandPool;
    VkCommandBuffer immCmdBuf;
    VkCommandBuffer eofCmdBuf;
    VkCommandBuffer graphicsCmdBuf;
    std::vector<UploadableBuffer> transfers;
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
    uint32_t presentQueueFamily;
    uint32_t graphicsQueueFamily;
    uint32_t transferQueueFamily;
};

struct RenderContext
{
    std::vector<VkSemaphore> imgAvailableSem;
    std::vector<VkSemaphore> renderDoneSem;
    std::vector<VkFence> fences;
    std::vector<VkCommandPool> commandPools;
    std::vector<VkCommandBuffer> commandBuffers;
};

enum class QueueFamily
{
    Transfer,
    Graphics,

    Size
};

struct VmaAllocationState
{
    VmaAllocation &allocation;         // out
    VmaAllocationInfo &allocationInfo; // out
    VmaMemoryUsage usage;
    VmaAllocationCreateFlags flags;
};