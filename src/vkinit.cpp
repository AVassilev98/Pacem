#include "shader.h"
#include "vkinit.h"

VkPipelineShaderStageCreateInfo VkInit::PipelineShaderStageCreateInfo(const Shader &shader)
{
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipelineShaderStageCreateInfo.stage = shader.m_moduleFlags;
    pipelineShaderStageCreateInfo.module = shader.m_module;
    pipelineShaderStageCreateInfo.pName = "main";

    return pipelineShaderStageCreateInfo;
}