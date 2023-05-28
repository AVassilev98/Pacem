#pragma once

#include "mesh.h"
#include "pipeline.h"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

class RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height){};
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) = 0;
    void addMesh(Mesh *mesh);

  public:
    std::vector<Framebuffer> m_framebuffers;
    Pipeline m_pipeline;

  protected:
    std::vector<Mesh *> m_meshes;
};

class MainRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;

  public:
    MainRenderPass(const std::span<Shader *> &shaders);
    ~MainRenderPass();

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    // virtual void resize(uint32_t width, uint32_t height) override;
    std::vector<Image> m_multisampledImages;
    std::vector<Image> m_depthImages;
    std::vector<Image *> m_swapchainImages;
};