#pragma once

#include "GpuResource.h"
#include "RenderPass.h"
#include <vulkan/vulkan_core.h>

class Gui : public RenderPass
{
  public:
    static Gui &Get();
    ~Gui();
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;

    VkRenderPass m_renderPass;

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    void collectImGuiFrameData();
    void setImGuiStyles();
    void setImGuiDockspace();

  private:
    Gui();
    VkDescriptorPool m_imguiDescriptorPool;
    PerFrameImage m_swapchainImages;
};