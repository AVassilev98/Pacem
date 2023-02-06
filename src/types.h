#pragma once
#include "GLFW/glfw3.h"
#include "vma.h"
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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
    glm::vec2 textureCoordinate;
};

struct Texture
{
    VkImage image;
    VmaAllocation allocation;
    VkImageView imageView;
};

struct PushConstants
{
    glm::mat4 M;
    glm::mat4 V;
    glm::mat4 P;
};

struct Mesh
{
    VkSampler sampler;

    // Total Mesh Buffer
    std::vector<Vertex> vertices;
    std::vector<glm::u32vec3> faces;
    AllocatedBuffer vkVertexBuffer;
    AllocatedBuffer vkIndexBuffer;

    // Per Submesh
    std::vector<VkDeviceSize> meshletVertexOffsets;
    std::vector<VkDeviceSize> meshletIndexOffsets;
    std::vector<VkDeviceSize> meshletIndexSizes;
    std::vector<VkDeviceSize> matIndex;

    // All Textures
    std::unordered_map<std::string, Texture> textures;

    // Per Material
    std::vector<VkDescriptorSet> matDescriptorSets;

    void drawMesh(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout);
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