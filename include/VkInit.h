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
    static VkPipelineLayout CreateVkPipelineLayout(const VkPipelineLayoutCreateInfo &pipelineLayoutCreateInfo);
    static VkPipeline CreateVkGraphicsPipeline(const VkGraphicsPipelineCreateInfo &graphicsPipelineCreateInfo);

    struct ComputePipelineState
    {
        const VkPipelineShaderStageCreateInfo &shader;
        const VkPipelineLayout &layout;
    };
    static VkPipeline CreateVkComputePipeline(const ComputePipelineState &state);

    // struct RenderPassState
    // {

    //     struct Attachment
    //     {
    //         VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //         VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //         VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //         VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //         VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //         VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //         const VkFormat &format;
    //         VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    //         const uint32_t &attachment;
    //         const VkImageLayout &referenceLayout;
    //     };

    //     struct SubpassDependency
    //     {
    //         uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
    //         uint32_t dstSubpass = 0;
    //         VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE;
    //         VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
    //         VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_NONE;
    //         VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
    //     };

    //     std::span<Attachment> colorAttachments = std::span<Attachment>();
    //     std::span<Attachment> inputAttachments = std::span<Attachment>();
    //     std::span<Attachment> depthAttachment = std::span<Attachment>();
    //     std::span<Attachment> resolveAttachments = std::span<Attachment>();
    //     std::span<SubpassDependency> dependencies;

    //     VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // };
    static VkRenderPass CreateVkRenderPass(const RenderPassState &state);

    struct SamplerState
    {
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBool32 anisotropyEnable = VK_FALSE;
        float maxAnisotropy = 0.0f;
        VkBool32 compareEnable = VK_FALSE;
        VkCompareOp compareOp = VK_COMPARE_OP_NEVER;
        float minLod = 0.0f;
        float maxLod = VK_LOD_CLAMP_NONE;
        VkBorderColor borderColor = {VK_BORDER_COLOR_INT_TRANSPARENT_BLACK};
        VkBool32 unnormalizedCoordinates = VK_FALSE;
    };
    static VkSampler CreateVkSampler(const SamplerState &state);

    struct BufferState
    {
        const VkDeviceSize &size;
        const VkBufferUsageFlags &usage;
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        const std::span<uint32_t> &queueFamilyIndices;

        const VmaAllocationState &allocation;
    };
    static VkBuffer CreateVkBuffer(const BufferState &state);

    struct ImageState
    {
        VkImageType type = VK_IMAGE_TYPE_2D;
        const VkFormat &format;
        const uint32_t &width;
        const uint32_t &height;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        const VkImageUsageFlags &usage;
        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        const std::span<uint32_t> &queueFamilyIndices;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        const VmaAllocationState &allocation;
    };
    static VkImage CreateVkImage(const ImageState &state);

    struct ImageViewState
    {
        const VkImage &image;
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        const VkFormat &format;
        VkComponentMapping components = {};
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t baseMipLevel = 0;
        uint32_t levelCount = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
    };
    static VkImageView CreateVkImageView(const ImageViewState &state);

    struct FramebufferState
    {
        const VkRenderPass &renderPass;
        const std::span<VkImageView> &attachments;
        const uint32_t &width;
        const uint32_t &height;
        uint32_t layers = 1;
    };
    static VkFramebuffer CreateVkFramebuffer(const FramebufferState &state);
};