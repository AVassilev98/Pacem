#pragma once
#include "types.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class Renderer
{
    friend class VkInit;

  public:
    static Renderer &Get();
    VkResult draw(Mesh &mesh, VkPipeline pipeline);
    void resize();
    bool exitSignal();
    void wait();
    ~Renderer();

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
    VkDescriptorPool m_descriptorPool = {};
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
    ImageGroupInfo getSwapchainImages();
    RenderContext createRenderContext();
    TransferQueue createTransferQueue();

    void destroyTransferQueue();
    void destroyRenderContext();
    void freeSwapchainImages();
    void destroyDepthBuffers();
    void destroyDebugMessenger();

    Renderer();

  private:
    uint64_t m_frameCount;
};