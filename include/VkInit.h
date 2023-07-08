#pragma once
#include "Common.h"
#include "Types.h"
#include "VkTypes.h"
#include "vulkan/vulkan.h"
#include <optional>
#include <span>
#include <vulkan/vulkan_core.h>

struct VkInit
{
    static VkPipelineShaderStageCreateInfo CreateVkPipelineShaderStageCreateInfo(const class Shader &shader);

    static VkDescriptorSetLayout CreateEmptyVkDescriptorSetLayout();
    static VkDescriptorSetLayout CreateVkDescriptorSetLayout(const std::span<const VkDescriptorSetLayoutBinding> &bindings);

    static VkDescriptorSetLayoutBinding CreateVkDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stages,
                                                                           uint32_t binding);
    static VkPipelineLayout CreateVkPipelineLayout(const PipelineLayoutState &pipelineLayoutCreateInfo);
    static VkPipeline CreateVkGraphicsPipeline(const VkGraphicsPipelineCreateInfo &graphicsPipelineCreateInfo);
    static VkPipeline CreateVkComputePipeline(const VkComputePipelineCreateInfo &state);
    static VkRenderPass CreateVkRenderPass(const RenderPassState &state);
    static VkSampler CreateVkSampler(const SamplerState &state);
    static VmaBuffer CreateVkBuffer(const BufferState &state);
    static VmaImage CreateVkImage(const ImageState &state);
    static VkImageView CreateVkImageView(const ImageViewState &state);
    static VkFramebuffer CreateVkFramebuffer(const FramebufferState &state);
};