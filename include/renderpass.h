#pragma once

#include "camera.h"
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

class EditorRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;

  public:
    EditorRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera);
    ~EditorRenderPass();
    GraphicsPipeline m_pipeline;

    std::vector<Image> m_multisampledImages;
    std::vector<Image> m_depthImages;

  private:
    const UserControlledCamera &m_cameraRef;
    void createFrameBuffers(VkRenderPass renderPass);
};

class MainRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;
    void declareImageDependency(std::vector<Image> &colorImages, std::vector<Image> &depthImages);

  public:
    MainRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera);
    ~MainRenderPass();
    GraphicsPipeline m_pipeline;
    std::vector<Image *> m_outputImages;

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    std::vector<Image *> m_multisampledImages;
    std::vector<Image *> m_depthImages;
    const UserControlledCamera &m_cameraRef;
};