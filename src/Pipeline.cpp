#include "Common.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "VkInit.h"
#include "VkTypes.h"
#include <array>
#include <assert.h>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

PipelineLayout::PipelineLayout(const PipelineLayoutState &&state)
    : m_pipelineState(state)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = toVkPipelineLayoutCreateInfo(std::move(state));
    VK_LOG_ERR(vkCreatePipelineLayout(Renderer::Get().getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));
}

GraphicsPipeline::GraphicsPipeline(const State &&state)
    : m_renderPass(state.renderPass)
{
    constexpr uint32_t maxShadersInPipeline = 4;
    constexpr auto requiredDynamicStates = std::to_array<VkDynamicState>({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});
    std::array<VkPipelineShaderStageCreateInfo, maxShadersInPipeline> shaderStages = {};
    uint32_t numShaderStages = 0;

    // clang-format off
    if(state.VS) shaderStages[numShaderStages++] = VkInit::CreateVkPipelineShaderStageCreateInfo(*state.VS);
    if(state.TS) shaderStages[numShaderStages++] = VkInit::CreateVkPipelineShaderStageCreateInfo(*state.TS);
    if(state.GS) shaderStages[numShaderStages++] = VkInit::CreateVkPipelineShaderStageCreateInfo(*state.GS);
    if(state.FS) shaderStages[numShaderStages++] = VkInit::CreateVkPipelineShaderStageCreateInfo(*state.FS);
    // clang-format on
    for (uint32_t i = 0; i < DSL_FREQ_COUNT; i++)
    {
        m_descriptorSetLayouts[i] = state.pipelineLayout.descSetLayouts[i];
    }
    m_pipelineLayout = VkInit::CreateVkPipelineLayout(toVkPipelineLayoutCreateInfo(std::move(state.pipelineLayout)));

    VkPipelineVertexInputStateCreateInfo vertexInputState = toVkPipelineVertexInputStateCreateInfo(state.vertexInputState);
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = toVkPipelineInputAssemblyStateCreateInfo(state.inputAssemblyState);
    VkPipelineTessellationStateCreateInfo tessellationState = toVkPipelineTessellationStateCreateInfo(state.tessellationState);
    VkPipelineViewportStateCreateInfo viewportState = toVkPipelineViewportStateCreateInfo(state.viewportState);
    VkPipelineRasterizationStateCreateInfo rasterizationState = toVkPipelineRasterizationStateCreateInfo(state.rasterizationState);
    VkPipelineMultisampleStateCreateInfo multiSampleState = toVkPipelineMultisampleStateCreateInfo(state.multisampleState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = toVkPipelineDepthStencilStateCreateInfo(state.depthStencilState);
    VkPipelineColorBlendStateCreateInfo colorBlendState = toVkPipelineColorBlendStateCreateInfo(state.colorBlendState);
    VkPipelineDynamicStateCreateInfo dynamicState = toVkPipelineDynamicStateCreateInfo(state.dynamicState);

    // create a list with both required and passed in dynamic states appended
    std::vector<VkDynamicState> dynamicStates(dynamicState.dynamicStateCount + requiredDynamicStates.size());
    for (uint32_t i = 0; i < dynamicState.dynamicStateCount; i++)
    {
        dynamicStates[i] = dynamicState.pDynamicStates[i];
    }
    for (uint32_t i = dynamicState.dynamicStateCount; i < dynamicState.dynamicStateCount + requiredDynamicStates.size(); i++)
    {
        dynamicStates[i] = requiredDynamicStates[i];
    }
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = numShaderStages,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = &tessellationState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multiSampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    m_pipeline = VkInit::CreateVkGraphicsPipeline(graphicsPipelineCreateInfo);
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

ComputePipeline::ComputePipeline(const State &&state)
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = VkInit::CreateVkPipelineShaderStageCreateInfo(state.CS);
    m_pipelineLayout = VkInit::CreateVkPipelineLayout(toVkPipelineLayoutCreateInfo(state.pipelineLayoutState));
    const std::span<const VkDescriptorSetLayout, DSL_FREQ_COUNT> &descriptorSets = state.pipelineLayoutState.descSetLayouts;
    std::copy(descriptorSets.begin(), descriptorSets.end(), m_descriptorSetLayouts.begin());

    VkInit::ComputePipelineState computePipelineState = {
        .shader = shaderStageCreateInfo,
        .layout = m_pipelineLayout,
    };
    m_pipeline = VkInit::CreateVkComputePipeline(computePipelineState);
}

void ComputePipeline::freeResources()
{
    Renderer &renderer = Renderer::Get();
    vkDestroyPipeline(renderer.getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(renderer.getDevice(), m_pipelineLayout, nullptr);
    for (VkDescriptorSetLayout &vkDescriptorSetLayout : m_descriptorSetLayouts)
    {
        vkDestroyDescriptorSetLayout(renderer.getDevice(), vkDescriptorSetLayout, nullptr);
    }
}