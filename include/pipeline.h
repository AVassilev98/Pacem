#pragma once

#include "common.h"
#include "shader.h"
#include <optional>
#include <span>
#include <vulkan/vulkan_core.h>

class GraphicsPipeline
{
  public:
    struct State
    {
        struct Dynamic
        {
            std::optional<VkRect2D> scissor = {};
            std::optional<VkViewport> viewport = {};
        };

        VkBool32 enableDepthTest = VK_TRUE;
        VkBool32 enableDepthWrite = VK_TRUE;
        const std::span<Shader *> &shaders;
        const std::span<VkDescriptorSetLayout, DSL_FREQ_COUNT> &layouts;
        const VkRenderPass &renderPass;
        std::span<VkPushConstantRange> pushConstantRanges;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        const std::span<VkVertexInputBindingDescription> &vertexBindingDescription;
        const std::span<VkVertexInputAttributeDescription> &vertexAttributeDescription;
        const std::span<VkPipelineColorBlendAttachmentState> &colorBlendAttachmentStates;
        VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
        Dynamic dynamicState = {};
    };
    GraphicsPipeline(const State &state);
    GraphicsPipeline() = default;

  public:
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    VkRenderPass m_renderPass;
    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> m_descriptorSetLayouts;
    void freeResources();

  private:
    State::Dynamic m_dynamicState;
};

class ComputePipeline
{
  public:
    struct State
    {
        const VkPipelineLayout &layout;
        const Shader &shader;
    };
    ComputePipeline(const State &state);
    ComputePipeline() = default;

  public:
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    void freeResources();
};