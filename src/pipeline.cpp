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

Pipeline::Pipeline(const State &state)
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

    std::copy(state.layouts.begin(), state.layouts.end(), m_descriptorSetLayouts.begin());
    m_pipelineLayout = VkInit::CreateVkPipelineLayout(m_descriptorSetLayouts, state.pushConstantRanges);

    VkPipelineTessellationStateCreateInfo tessellationStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentStateInfo,
    };

    VkPipelineMultisampleStateCreateInfo msaaStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = state.sampleCount,
        .minSampleShading = 1.0f,
    };

    VkExtent2D windowExtent = Renderer::Get().getWindowExtent();
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

    VkInit::PipelineState pipelineState = {
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
    m_pipeline = VkInit::CreateVkPipeline(pipelineState);
    m_dynamicState = state.dynamicState;
}
