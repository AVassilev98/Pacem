#pragma once

#include "pipeline.h"
#include <stdexcept>
#include <vector>

class RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height){};

  public:
    std::vector<Framebuffer> m_framebuffers;
    Pipeline m_pipeline;
};

class MainRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;

  public:
    MainRenderPass(const std::span<Shader *> &shaders);

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    // virtual void resize(uint32_t width, uint32_t height) override;
    std::vector<Image> m_multisampledImages;
    std::vector<Image> m_depthImages;
    std::vector<Image *> m_swapchainImages;
};