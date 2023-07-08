#pragma once

#include "Common.h"
#include "Shader.h"
#include "VkInit.h"
#include "VkTypes.h"
#include <optional>
#include <span>
#include <vulkan/vulkan_core.h>

struct PipelineLayout
{
    friend class GraphicsPipeline;
    friend class ComputePipeline;

    PipelineLayoutState m_pipelineState;
    VkPipelineLayout m_pipelineLayout;

  private:
    PipelineLayout(const PipelineLayoutState &&state);
};

class GraphicsPipeline
{
  public:
    struct State
    {
        Shader *VS = nullptr;
        Shader *TS = nullptr;
        Shader *GS = nullptr;
        Shader *FS = nullptr;
        VertexInputState vertexInputState;
        InputAssemblyState inputAssemblyState = {};
        TessellationState tessellationState = {};
        ViewportState viewportState = {};
        RasterizationState rasterizationState = {};
        MultisampleState multisampleState = {};
        DepthStencilState depthStencilState = {};
        ColorBlendState colorBlendState = {};
        DynamicState dynamicState = {};
        PipelineLayoutState pipelineLayout;
        const VkRenderPass &renderPass;
    };
    GraphicsPipeline(const State &&state);
    GraphicsPipeline() = default;

  public:
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
    VkRenderPass m_renderPass;
    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> m_descriptorSetLayouts;
    void freeResources();

  private:
};

class ComputePipeline
{
  public:
    struct State
    {
        PipelineLayoutState pipelineLayoutState;
        const Shader &CS;
    };
    ComputePipeline(const State &&state);
    ComputePipeline() = default;

  public:
    VkPipelineLayout m_pipelineLayout;
    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> m_descriptorSetLayouts;
    VkPipeline m_pipeline;
    void freeResources();
};
