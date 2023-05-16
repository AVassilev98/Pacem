#pragma once
#include "vulkan/vulkan.h"

struct VkInit
{
    static VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const class Shader &shader);
};