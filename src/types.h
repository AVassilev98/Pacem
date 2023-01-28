#pragma once
#include "GLFW/glfw3.h"
#include "vma.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <vulkan/vulkan.h>

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

struct ImageGroupInfo
{
    std::vector<VkImage> msaaImages;
    std::vector<VkImageView> msaaImageViews;
    std::vector<VmaAllocation> msaaImageAllocations;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> frameBuffers;
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
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<glm::u32vec3> faces;
    AllocatedBuffer vkVertexBuffer;
    AllocatedBuffer vkIndexBuffer;
    std::vector<VkDeviceSize> meshletVertexOffsets;
    std::vector<VkDeviceSize> meshletIndexOffsets;
    std::vector<VkDeviceSize> meshletIndexSizes;

    void drawMesh(VkCommandBuffer cmdBuf);
};

struct DepthBuffer
{
    VkImage depthBuf;
    VmaAllocation imageAlloc;
    VkImageView depthView;
};

struct Uploadable
{
    enum class Type
    {
        Buffer,
        Image,
    };

    Type uploadType;
    union
    {
        std::pair<VkBuffer, VkBuffer> bufferPair;
        std::pair<VkImage, VkImage> imagePair;
    };
};

struct TransferQueue
{
    VkQueue queue;
    VkCommandPool commandPool;
    VkCommandBuffer immCmdBuf;
    VkCommandBuffer eofCmdBuf;
    std::vector<Uploadable> transfers;
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