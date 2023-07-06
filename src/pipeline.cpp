#include "common.h"
#include "pipeline.h"
#include "renderer.h"
#include "vkinit.h"
#include <array>
#include <assert.h>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

GraphicsPipeline::GraphicsPipeline(const State &state)
    : m_renderPass(state.renderPass)
{
    constexpr unsigned maxShadersInPipeline = 5;
    std::array<VkPipelineShaderStageCreateInfo, maxShadersInPipeline> shaderStages;
    unsigned numShaderStages = 0;

    for (const Shader *shader : state.shaders)
    {
        assert(shader && "Shader should not be null");
        shaderStages[numShaderStages] = VkInit::CreateVkPipelineShaderStageCreateInfo(*shader);
        numShaderStages++;
    }

    for (uint32_t i = 0; i < DSL_FREQ_COUNT; i++)
    {
        m_descriptorSetLayouts[i] = state.layouts[i];
    }
    m_pipelineLayout = VkInit::CreateVkPipelineLayout(m_descriptorSetLayouts, state.pushConstantRanges);

    VkPipelineTessellationStateCreateInfo tessellationStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(state.colorBlendAttachmentStates.size()),
        .pAttachments = state.colorBlendAttachmentStates.data(),
    };

    VkPipelineMultisampleStateCreateInfo msaaStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = state.sampleCount,
        .minSampleShading = 1.0f,
    };

    VkExtent2D windowExtent = Renderer::Get().getDrawAreaExtent();
    VkRect2D scissor = state.dynamicState.scissor.has_value() ? state.dynamicState.scissor.value()
                                                              : VkRect2D{
                                                                  .offset = {},
                                                                  .extent = windowExtent,
                                                              };
    VkViewport viewport = state.dynamicState.viewport.has_value() ? state.dynamicState.viewport.value()
                                                                  : VkViewport{
                                                                      .x = 0,
                                                                      .y = 0,
                                                                      .width = static_cast<float>(windowExtent.width),
                                                                      .height = static_cast<float>(windowExtent.height),
                                                                      .minDepth = 0.0f,
                                                                      .maxDepth = 1.0f,
                                                                  };

    auto dynamicStates = std::to_array({
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    });

    VkInit::GraphicsPipelineState pipelineState = {
        .layout = m_pipelineLayout,
        .renderPass = state.renderPass,
        .shaderInfo = std::span(shaderStages.data(), numShaderStages),
        .vertexInputBindingDescription = state.vertexBindingDescription,
        .vertexInputAttributeDescription = state.vertexAttributeDescription,
        .msaaStateInfo = msaaStateInfo,
        .colorBlendStateInfo = colorBlendStateInfo,
        .viewports = std::span(&viewport, 1),
        .scissors = std::span(&scissor, 1),
        .dynamicStates = dynamicStates,
    };
    pipelineState.depthStencilInfo.depthTestEnable = state.enableDepthTest;
    pipelineState.depthStencilInfo.depthWriteEnable = state.enableDepthWrite;

    m_pipeline = VkInit::CreateVkGraphicsPipeline(pipelineState);
    m_dynamicState = state.dynamicState;
}

void GraphicsPipeline::freeResources()
{
    Renderer &renderer = Renderer::Get();

    vkDestroyPipeline(renderer.getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(renderer.getDevice(), m_pipelineLayout, nullptr);
    vkDestroyRenderPass(renderer.getDevice(), m_renderPass, nullptr);
    for (VkDescriptorSetLayout descriptorSetLayout : m_descriptorSetLayouts)
    {
        vkDestroyDescriptorSetLayout(renderer.getDevice(), descriptorSetLayout, nullptr);
    }
}

ComputePipeline::ComputePipeline(const State &state)
    : m_pipelineLayout(state.layout)
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = VkInit::CreateVkPipelineShaderStageCreateInfo(state.shader);

    VkInit::ComputePipelineState computePipelineState = {
        .shader = shaderStageCreateInfo,
        .layout = state.layout,
    };
    m_pipeline = VkInit::CreateVkComputePipeline(computePipelineState);
}

void ComputePipeline::freeResources()
{
    Renderer &renderer = Renderer::Get();
    vkDestroyPipeline(renderer.getDevice(), m_pipeline, nullptr);
}