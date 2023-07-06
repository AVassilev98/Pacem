#pragma once

#include "camera.h"
#include "mesh.h"
#include "pipeline.h"
#include "renderer.h"
#include "types.h"
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
    PerFrameFramebuffer m_framebuffers;

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

    PerFrameImage m_multisampledImages;
    PerFrameImage m_depthImages;

  private:
    const UserControlledCamera &m_cameraRef;
    void createFrameBuffers(VkRenderPass renderPass);
};

struct GBuffer
{
    PerFrameImage m_diffuseBuffers;
    PerFrameImage m_normalBuffers;
    PerFrameImage m_depthImages;
};

class DeferredRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;
    void declareImageDependency(PerFrameImage &depthImages);

  public:
    DeferredRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera);
    ~DeferredRenderPass();
    GraphicsPipeline m_pipeline;
    GBuffer m_gBuffer;

  private:
    void createFrameBuffers(VkRenderPass renderPass);
    void updateGBuffer();

    PerFrameImage m_diffuseBuffers;
    PerFrameImage m_normalBuffers;

    PerFrameImage m_outputImages;
    PerFrameImage m_depthImages;
    const UserControlledCamera &m_cameraRef;
};

class ShadingRenderPass : public RenderPass
{
  public:
    virtual void resize(uint32_t width, uint32_t height) override;
    virtual void draw(VkCommandBuffer buffer, uint32_t frameIdx) override;
    void declareGBufferDependency(GBuffer &gBuffer);

  public:
    ShadingRenderPass(const Shader &lightCullShader, const Shader &lightingShader, const UserControlledCamera &camera);
    ~ShadingRenderPass();

  private:
    void createDescriptorSets();
    void createImages();
    void updateDescriptorSet(VkCommandBuffer commandBuffer, uint32_t frameIdx);

  private:
    GBuffer *m_gBuffer = nullptr;
    PerFrameImage m_outputImages;

    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> m_lightingDescriptorSetLayouts;
    std::vector<std::array<VkDescriptorSet, DSL_FREQ_COUNT>> m_lightingDescriptorSets
        = std::vector<std::array<VkDescriptorSet, DSL_FREQ_COUNT>>(Renderer::Get().numFramesInFlight());

    ComputePipeline m_lightCullPipeline;
    ComputePipeline m_shadingPipeline;

    VkPipelineLayout m_lightCullPipelineLayout;
    VkPipelineLayout m_shadingPipelineLayout;

    const UserControlledCamera &m_cameraRef;
};