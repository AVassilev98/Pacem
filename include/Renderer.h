#pragma once
#include <functional>
#include <vulkan/vulkan_core.h>

#include "GpuResource.h"
#include "Mesh.h"
#include "PerFrameResource.h"
#include "Pipeline.h"
#include "ResourcePool.h"
#include "Types.h"
#include "backends/imgui_impl_vulkan.h"

class Renderer
{
  public:
    static constexpr uint32_t MaxFramesInFlight = 3;

  public:
    static Renderer &Get();
    VkDevice getDevice();
    VmaAllocator getAllocator();
    PerFrameImage getSwapchainImages();
    VkDescriptorPool getDescriptorPool();

    void initImGuiGlfwVulkan(VkRenderPass renderPass);

    VkResult draw();
    VkExtent2D getDrawAreaExtent();
    bool exitSignal();
    uint32_t getQueueFamilyIdx(QueueFamily family);
    uint32_t numFramesInFlight();
    uint32_t curFrame();
    uint32_t frameCount();

    VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout);
    const SwapchainInfo &getSwapchainInfo();
    void addRenderPass(RenderPass *renderPass);

    void wait();
    ~Renderer();

    Handle<Buffer> uploadBufferToGpu(VkBuffer src, const Buffer::State &&dstState);
    Handle<Image> uploadImageToGpu(VkBuffer src, const Image::State &&dstState);

    struct DescriptorUpdateState
    {
        const VkDescriptorSet &descriptorSet;
        VkImageView imageView = VK_NULL_HANDLE;
        const VkImageLayout &imageLayout;
        const VkSampler &imageSampler;
        uint32_t dstArrayElement = 0;
        uint32_t binding = 0;
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    };
    void updateDescriptor(const DescriptorUpdateState &descriptorUpdateState);

    struct DescriptorWriteState
    {
        const VkDescriptorSet &descriptorSet;
        VkImageView imageView = VK_NULL_HANDLE;
        const VkImageLayout &imageLayout;
        const VkSampler &imageSampler;
        uint32_t dstArrayElement = 0;
        uint32_t binding = 0;
        VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    };
    void writeDescriptor(const DescriptorWriteState &descriptorUpdateState);

    void transferImmediate(std::function<void(VkCommandBuffer cmd)> &&function);
    void graphicsImmediate(std::function<void(VkCommandBuffer cmd)> &&function);
    Handle<Image> uploadTextureToGpu(const std::span<uint8_t> &buf, uint32_t width, uint32_t height);
    Handle<Buffer> uploadCpuBufferToGpu(const std::span<uint8_t> &buf, VkBufferUsageFlags usage);

    Handle<Image> create(const Image::State &&state);
    Image *get(Handle<Image> handle);
    void destroy(Handle<Image> handle);

    Handle<Buffer> create(const Buffer::State &&state);
    Buffer *get(Handle<Buffer> handle);
    void destroy(Handle<Buffer> handle);

    Handle<Framebuffer> create(const Framebuffer::State &&state);
    Framebuffer *get(Handle<Framebuffer> handle);
    void destroy(Handle<Framebuffer> handle);

    Handle<GraphicsPipeline> create(const GraphicsPipeline::State &&state);
    GraphicsPipeline *get(Handle<GraphicsPipeline> handle);

    Handle<ComputePipeline> create(const ComputePipeline::State &&state);
    ComputePipeline *get(Handle<ComputePipeline> handle);

    void createSwapchainImages();

  private:
    Pool<Image> m_imagePool;
    Pool<Buffer> m_bufferPool;
    Pool<Framebuffer> m_framebufferPool;

    Pool<GraphicsPipeline> m_graphicsPipelinePool;
    Pool<ComputePipeline> m_computePipelinePool;

    VkInstance m_instance = {};
    VkDebugUtilsMessengerEXT m_debugMessenger;
    PhysDeviceInfo m_physDeviceInfo = {};
    WindowInfo m_windowInfo = {};
    VkSampleCountFlagBits m_numSamples = VK_SAMPLE_COUNT_1_BIT;
    DeviceInfo m_deviceInfo = {};
    VmaAllocator m_vmaAllocator = {};
    SwapchainInfo m_swapchainInfo = {};
    std::vector<VkDescriptorPool> m_descriptorPools;
    PerFrameImage m_swapchainImages;
    RenderContext m_renderContext = {};
    TransferQueue m_transferQueue = {};
    std::vector<RenderPass *> m_renderPasses = {};
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    uint64_t m_frameCount;

  private:
    VkExtent2D getWindowExtent();
    void initialize();
    const WindowInfo createWindow();
    VkInstance createInstance();
    VkDebugUtilsMessengerEXT createDebugMessenger();
    const PhysDeviceInfo createPhysicalDeviceInfo();
    VkSampleCountFlagBits getMaxSamples();
    const DeviceInfo createDevice();
    const VmaAllocator createVmaAllocator();
    SwapchainInfo createSwapchain(VkSwapchainKHR swapchain);
    VkDescriptorPool createDescriptorPool();
    void destroyDescriptorPools();
    RenderContext createRenderContext();
    TransferQueue createTransferQueue();

    void handleResize();

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
};