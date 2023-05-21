#pragma once
#include "mesh.h"
#include "types.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class Renderer
{

  public:
    static Renderer &Get();
    VkResult draw(Mesh &mesh, VkPipeline pipeline);
    VkExtent2D getWindowExtent();
    void resize();
    bool exitSignal();
    uint32_t getQueueFamilyIdx(QueueFamily family);
    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);
    void wait();
    ~Renderer();

    VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout layout);
    Buffer uploadBufferToGpu(VkBuffer src, Buffer::State &dstState);
    Image uploadImageToGpu(VkBuffer src, Image::State &dstState);
    void updateDescriptor(VkDescriptorSet descriptorSet, const VkDescriptorImageInfo &descriptorImageInfo, uint32_t binding);

  public:
    // Template functions
    template <typename T> Image uploadTextureToGpu(const std::span<T> &buf, uint32_t width, uint32_t height)
    {
        auto bufQueueFamilies = std::to_array({QueueFamily::Transfer});
        uint32_t bufferSize = buf.size() * sizeof(T);
        Buffer::State stagingBufferState = {
            .size = bufferSize,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .families = bufQueueFamilies,
            .vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        };
        Buffer stagingBuffer(stagingBufferState);
        memcpy(stagingBuffer.m_allocationInfo.pMappedData, buf.data(), bufferSize);

        auto texQueueFamilies = std::to_array({QueueFamily::Graphics, QueueFamily::Transfer});
        Image::State dstState = {
            .width = width,
            .height = height,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .families = texQueueFamilies,
        };

        return uploadImageToGpu(stagingBuffer.m_buffer, dstState);
    }

    template <typename T> Buffer uploadCpuBufferToGpu(const std::span<T> &buf, VkBufferUsageFlags usage)
    {
        VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingBufferInfo.size = buf.size() * sizeof(T);
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto queueFamilyIndices = std::to_array({m_deviceInfo.graphicsQueueFamily, m_deviceInfo.transferQueueFamily});
        stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        stagingBufferInfo.queueFamilyIndexCount = queueFamilyIndices.size();

        VmaAllocationCreateInfo vmaStagingBufAllocInfo = {};
        vmaStagingBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        vmaStagingBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuf;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo stagingAllocInfo;
        VK_LOG_ERR(
            vmaCreateBuffer(m_vmaAllocator, &stagingBufferInfo, &vmaStagingBufAllocInfo, &stagingBuf, &stagingAlloc, &stagingAllocInfo));
        memcpy(stagingAllocInfo.pMappedData, buf.data(), stagingBufferInfo.size);

        auto queueFamilies = std::to_array({QueueFamily::Graphics, QueueFamily::Transfer});
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        Buffer::State dstState = {
            .size = static_cast<uint32_t>(stagingBufferInfo.size),
            .usage = usage,
            .families = queueFamilies,
        };
        return uploadBufferToGpu(stagingBuf, dstState);
    }

    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debugMessenger;
    PhysDeviceInfo m_physDeviceInfo = {};
    WindowInfo m_windowInfo = {};
    VkSampleCountFlagBits m_numSamples = VK_SAMPLE_COUNT_1_BIT;
    DeviceInfo m_deviceInfo = {};
    VmaAllocator m_vmaAllocator = {};
    SwapchainInfo m_swapchainInfo = {};
    std::vector<DepthBuffer> m_depthBuffers = {};
    VkDescriptorSetLayout m_descriptorSetLayout = {};
    VkPipelineLayout m_pipelineLayout = {};
    std::vector<VkDescriptorPool> m_descriptorPools;
    VkRenderPass m_renderPass = {};
    ImageGroupInfo m_swapchainImages = {};
    RenderContext m_renderContext = {};
    TransferQueue m_transferQueue = {};

  private:
    void initialize();
    const WindowInfo createWindow();
    VkInstance createInstance();
    VkDebugUtilsMessengerEXT createDebugMessenger();
    const PhysDeviceInfo createPhysicalDeviceInfo();
    VkSampleCountFlagBits getMaxSamples();
    const DeviceInfo createDevice();
    const VmaAllocator createVmaAllocator();
    SwapchainInfo createSwapchain();
    VkRenderPass createRenderPass();
    std::vector<DepthBuffer> createDepthBuffers();
    VkDescriptorSetLayout createDescriptorSetLayout();
    VkPipelineLayout createPipelineLayout();
    VkDescriptorPool createDescriptorPool();
    void destroyDescriptorPools();
    ImageGroupInfo getSwapchainImages();
    RenderContext createRenderContext();
    TransferQueue createTransferQueue();

    struct UploadInfo
    {
        VkBuffer &src;
        Image &dst;
    };
    void transferImageImmediate(UploadInfo &info);

    void destroyTransferQueue();
    void destroyRenderContext();
    void freeSwapchainImages();
    void destroyDepthBuffers();
    void destroyDebugMessenger();

    Renderer();

  private:
    uint64_t m_frameCount;
};