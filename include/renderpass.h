#pragma once

#include "mesh.h"
#include "pipeline.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

class RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height){};
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) = 0;
    void addMesh(Mesh *mesh);
    void fulfillRenderPassDependencies(VkCommandBuffer cmd, uint32_t frameIdx);

  public:
    std::vector<Framebuffer> m_framebuffers;

  protected:
    std::vector<Mesh *> m_meshes;
    std::vector<std::function<void(VkCommandBuffer, uint32_t)>> m_dependencies;
};

class MainRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;

  public:
    MainRenderPass(const std::span<Shader *> &shaders);
    ~MainRenderPass();
    Pipeline m_pipeline;
    std::vector<Image> m_outputImages;

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    std::vector<Image> m_multisampledImages;
    std::vector<Image> m_depthImages;
};