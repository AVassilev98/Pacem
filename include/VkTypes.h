#pragma once
#include "Common.h"
#include "vma.h"
#include "vulkan/vulkan.h"
#include <array>
#include <cassert>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

struct VertexInputBindingDescription
{
    const uint32_t binding;
    const uint32_t stride;
    const VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
};
inline VkVertexInputBindingDescription toVkInputBindingDescription(const VertexInputBindingDescription &toConvert)
{
    return VkVertexInputBindingDescription{
        .binding = toConvert.binding,
        .stride = toConvert.stride,
        .inputRate = toConvert.inputRate,
    };
}

struct VertexInputAttributeDescription
{
    const uint32_t location;
    const uint32_t binding;
    const VkFormat format;
    const uint32_t offset;
};
inline VkVertexInputAttributeDescription toVkInputAttributeDescription(const VertexInputAttributeDescription &toConvert)
{
    return VkVertexInputAttributeDescription{
        .location = toConvert.location,
        .binding = toConvert.binding,
        .format = toConvert.format,
        .offset = toConvert.offset,
    };
}

struct VertexInputState
{
    const VkPipelineVertexInputStateCreateFlags flags = 0;
    const std::span<const VertexInputBindingDescription> vertexBindingDescs;
    const std::span<const VertexInputAttributeDescription> vertexAttributeDescs;
};
inline VkPipelineVertexInputStateCreateInfo toVkPipelineVertexInputStateCreateInfo(const VertexInputState &toConvert)
{
    assert(sizeof(VkVertexInputBindingDescription) == sizeof(VertexInputBindingDescription) && "Size mismatch: This should NEVER fail");
    assert(sizeof(VertexInputAttributeDescription) == sizeof(VkVertexInputAttributeDescription) && "Size mismatch: This should NEVER fail");

    return VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(toConvert.vertexBindingDescs.size()),
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription *)toConvert.vertexBindingDescs.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(toConvert.vertexAttributeDescs.size()),
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription *)toConvert.vertexAttributeDescs.data(),
    };
}

struct InputAssemblyState
{
    const VkPipelineInputAssemblyStateCreateFlags flags = 0;
    const VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    const VkBool32 primitiveRestartEnable = VK_FALSE;
};
inline VkPipelineInputAssemblyStateCreateInfo toVkPipelineInputAssemblyStateCreateInfo(const InputAssemblyState &toConvert)
{
    return VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .topology = toConvert.topology,
        .primitiveRestartEnable = toConvert.primitiveRestartEnable,
    };
}

struct TessellationState
{
    const VkPipelineTessellationStateCreateFlags flags = 0;
    const uint32_t patchControlPoints = 0;
};
inline VkPipelineTessellationStateCreateInfo toVkPipelineTessellationStateCreateInfo(const TessellationState &toConvert)
{
    return VkPipelineTessellationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .patchControlPoints = toConvert.patchControlPoints,
    };
}

struct RasterizationState
{
    const VkPipelineRasterizationStateCreateFlags flags = 0;
    const VkBool32 depthClampEnable = VK_FALSE;
    const VkBool32 rasterizerDiscardEnable = VK_FALSE;
    const VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    const VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    const VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    const VkBool32 depthBiasEnable = VK_TRUE;
    const float depthBiasConstantFactor = 0.0f;
    const float depthBiasClamp = 0.0f;
    const float depthBiasSlopeFactor = 0.0f;
    const float lineWidth = 1.0f;
};
inline VkPipelineRasterizationStateCreateInfo toVkPipelineRasterizationStateCreateInfo(const RasterizationState &toConvert)
{
    return VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .depthClampEnable = toConvert.depthClampEnable,
        .rasterizerDiscardEnable = toConvert.rasterizerDiscardEnable,
        .polygonMode = toConvert.polygonMode,
        .cullMode = toConvert.cullMode,
        .frontFace = toConvert.frontFace,
        .depthBiasEnable = toConvert.depthBiasEnable,
        .depthBiasConstantFactor = toConvert.depthBiasConstantFactor,
        .depthBiasClamp = toConvert.depthBiasClamp,
        .depthBiasSlopeFactor = toConvert.depthBiasSlopeFactor,
        .lineWidth = toConvert.lineWidth,
    };
}

struct MultisampleState
{
    const VkPipelineMultisampleStateCreateFlags flags = 0;
    const VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    const VkBool32 sampleShadingEnable = VK_FALSE;
    const float minSampleShading = 1.0f;
    const VkSampleMask *pSampleMask = VK_NULL_HANDLE;
    const VkBool32 alphaToCoverageEnable = VK_FALSE;
    const VkBool32 alphaToOneEnable = VK_FALSE;
};
inline VkPipelineMultisampleStateCreateInfo toVkPipelineMultisampleStateCreateInfo(const MultisampleState &toConvert)
{
    return VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .rasterizationSamples = toConvert.rasterizationSamples,
        .sampleShadingEnable = toConvert.sampleShadingEnable,
        .minSampleShading = toConvert.minSampleShading,
        .pSampleMask = toConvert.pSampleMask,
        .alphaToCoverageEnable = toConvert.alphaToCoverageEnable,
        .alphaToOneEnable = toConvert.alphaToOneEnable,
    };
}

struct StencilOpState
{
    const VkStencilOp failOp = VK_STENCIL_OP_ZERO;
    const VkStencilOp passOp = VK_STENCIL_OP_KEEP;
    const VkStencilOp depthFailOp = VK_STENCIL_OP_ZERO;
    const VkCompareOp compareOp = VK_COMPARE_OP_NEVER;
    const uint32_t compareMask = 0;
    const uint32_t writeMask = 0;
    const uint32_t reference = 0;
};
inline VkStencilOpState toVkStencilOpState(const StencilOpState &toConvert)
{
    return VkStencilOpState{
        .failOp = toConvert.failOp,
        .passOp = toConvert.passOp,
        .depthFailOp = toConvert.depthFailOp,
        .compareOp = toConvert.compareOp,
        .compareMask = toConvert.compareMask,
        .writeMask = toConvert.writeMask,
        .reference = toConvert.reference,
    };
}

struct DepthStencilState
{
    const VkPipelineDepthStencilStateCreateFlags flags = 0;
    const VkBool32 depthTestEnable = VK_TRUE;
    const VkBool32 depthWriteEnable = VK_TRUE;
    const VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    const VkBool32 depthBoundsTestEnable = VK_TRUE;
    const VkBool32 stencilTestEnable = VK_FALSE;
    const StencilOpState front = {};
    const StencilOpState back = {};
    const float minDepthBounds = 0.01f;
    const float maxDepthBounds = 1.0f;
};
inline VkPipelineDepthStencilStateCreateInfo toVkPipelineDepthStencilStateCreateInfo(const DepthStencilState &toConvert)
{
    return VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .depthTestEnable = toConvert.depthTestEnable,
        .depthWriteEnable = toConvert.depthWriteEnable,
        .depthCompareOp = toConvert.depthCompareOp,
        .depthBoundsTestEnable = toConvert.depthBoundsTestEnable,
        .stencilTestEnable = toConvert.stencilTestEnable,
        .front = toVkStencilOpState(std::move(toConvert.front)),
        .back = toVkStencilOpState(std::move(toConvert.back)),
        .minDepthBounds = toConvert.minDepthBounds,
        .maxDepthBounds = toConvert.maxDepthBounds,
    };
}

struct ColorBlendAttachmentState
{
    VkBool32 blendEnable = VK_FALSE;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
};

struct ColorBlendState
{
    VkPipelineColorBlendStateCreateFlags flags = 0;
    VkBool32 logicOpEnable = VK_FALSE;
    VkLogicOp logicOp = VK_LOGIC_OP_CLEAR;
    std::span<const ColorBlendAttachmentState> colorBlendAttachmentStates = {};
    std::array<float, 4> blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
};

inline VkPipelineColorBlendStateCreateInfo toVkPipelineColorBlendStateCreateInfo(const ColorBlendState &toConvert)
{
    assert(sizeof(VkPipelineColorBlendAttachmentState) == sizeof(ColorBlendAttachmentState) && "Size mismatch: This should NEVER fail");
    return VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .logicOpEnable = toConvert.logicOpEnable,
        .logicOp = toConvert.logicOp,
        .attachmentCount = static_cast<uint32_t>(toConvert.colorBlendAttachmentStates.size()),
        .pAttachments = (VkPipelineColorBlendAttachmentState *)toConvert.colorBlendAttachmentStates.data(),
        .blendConstants = {
            toConvert.blendConstants[0],
            toConvert.blendConstants[1],
            toConvert.blendConstants[2],
            toConvert.blendConstants[3],
        },
    };
}

struct DynamicState
{
    VkPipelineDynamicStateCreateFlags flags = 0;
    std::span<VkDynamicState> dynamicStates = {};
};
inline VkPipelineDynamicStateCreateInfo toVkPipelineDynamicStateCreateInfo(const DynamicState &toConvert)
{
    return VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .dynamicStateCount = static_cast<uint32_t>(toConvert.dynamicStates.size()),
        .pDynamicStates = toConvert.dynamicStates.data(),
    };
}

struct PipelineLayoutState
{
    const std::span<const VkDescriptorSetLayout, DSL_FREQ_COUNT> descSetLayouts;
    const std::span<const VkPushConstantRange> pushConstantRanges = std::span<VkPushConstantRange>();
};
inline VkPipelineLayoutCreateInfo toVkPipelineLayoutCreateInfo(const PipelineLayoutState &toConvert)
{
    return VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .setLayoutCount = static_cast<uint32_t>(toConvert.descSetLayouts.size()),
        .pSetLayouts = toConvert.descSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(toConvert.pushConstantRanges.size()),
        .pPushConstantRanges = toConvert.pushConstantRanges.data(),
    };
}

struct ViewportState
{
    VkPipelineViewportStateCreateFlags flags = 0;
    uint32_t viewportCount = 1;
    VkViewport *pViewports = nullptr;
    uint32_t scissorCount = 1;
    VkRect2D *pScissors = nullptr;
};
inline VkPipelineViewportStateCreateInfo toVkPipelineViewportStateCreateInfo(const ViewportState &toConvert)
{
    return VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = toConvert.flags,
        .viewportCount = toConvert.viewportCount,
        .pViewports = toConvert.pViewports,
        .scissorCount = toConvert.scissorCount,
        .pScissors = toConvert.pScissors,
    };
}

struct AttachmentDescription
{
    const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    const VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    const VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    const VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    const VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    const VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    const VkFormat &format;
    const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    const uint32_t &attachment;
    const VkImageLayout &referenceLayout;
};

inline VkAttachmentDescription toVkAttachmentDescription(const AttachmentDescription &toConvert)
{
    return VkAttachmentDescription{
        .flags = 0,
        .format = toConvert.format,
        .samples = toConvert.samples,
        .loadOp = toConvert.loadOp,
        .storeOp = toConvert.storeOp,
        .stencilLoadOp = toConvert.stencilLoadOp,
        .stencilStoreOp = toConvert.stencilStoreOp,
        .initialLayout = toConvert.initialLayout,
        .finalLayout = toConvert.finalLayout,
    };
}

inline VkAttachmentReference toVkAttachmentReference(const AttachmentDescription &toConvert)
{
    return VkAttachmentReference{
        .attachment = toConvert.attachment,
        .layout = toConvert.referenceLayout,
    };
}

struct SubpassDependency
{
    const uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
    const uint32_t dstSubpass = 0;
    const VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_NONE;
    const VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_NONE;
    const VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
    const VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
};
inline VkSubpassDependency toVkSubpassDependency(const SubpassDependency &toConvert)
{
    return VkSubpassDependency{
        .srcSubpass = toConvert.srcSubpass,
        .dstSubpass = toConvert.dstSubpass,
        .srcStageMask = toConvert.srcStageMask,
        .dstStageMask = toConvert.dstStageMask,
        .srcAccessMask = toConvert.srcAccessMask,
        .dstAccessMask = toConvert.dstAccessMask,
    };
}

struct RenderPassState
{
    const std::span<const AttachmentDescription> colorAttachments;
    const std::span<const AttachmentDescription> inputAttachments;
    const std::span<const AttachmentDescription> depthAttachment;
    const std::span<const AttachmentDescription> resolveAttachments;
    const std::span<const SubpassDependency> dependencies;
    VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

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
inline VkSamplerCreateInfo toVkSamplerCreateInfo(const SamplerState &toConvert)
{
    return VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = toConvert.magFilter,
        .minFilter = toConvert.minFilter,
        .mipmapMode = toConvert.mipmapMode,
        .addressModeU = toConvert.addressModeU,
        .addressModeV = toConvert.addressModeV,
        .addressModeW = toConvert.addressModeW,
        .mipLodBias = 0.0f,
        .anisotropyEnable = toConvert.anisotropyEnable,
        .maxAnisotropy = toConvert.maxAnisotropy,
        .compareEnable = toConvert.compareEnable,
        .compareOp = toConvert.compareOp,
        .minLod = toConvert.minLod,
        .maxLod = toConvert.maxLod,
        .borderColor = toConvert.borderColor,
        .unnormalizedCoordinates = toConvert.unnormalizedCoordinates,
    };
}

struct BufferState
{
    const VkDeviceSize size = 0;
    const VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    const std::span<const uint32_t> queueFamilyIndices;
    const VmaAllocationCreateInfo allocation;
};
inline VkBufferCreateInfo toVkBufferCreateInfo(const BufferState &toConvert)
{
    assert(toConvert.queueFamilyIndices.size() && "Buffer must belong to a queue!");
    return VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .size = toConvert.size,
        .usage = toConvert.usage,
        .sharingMode = toConvert.sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(toConvert.queueFamilyIndices.size()),
        .pQueueFamilyIndices = toConvert.queueFamilyIndices.data(),
    };
}
struct VmaBuffer
{
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    VkBuffer buffer = VK_NULL_HANDLE;
};

struct ImageState
{
    const VkImageType type = VK_IMAGE_TYPE_2D;
    const VkFormat &format;
    const uint32_t &width;
    const uint32_t &height;
    const uint32_t depth = 1;
    const uint32_t mipLevels = 1;
    const uint32_t arrayLayers = 1;
    const VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    const VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    const std::span<const uint32_t> queueFamilyIndices;
    const VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    const VmaAllocationCreateInfo allocation;
};
inline VkImageCreateInfo toVkImageCreateInfo(const ImageState &toConvert)
{
    assert(toConvert.queueFamilyIndices.size() && "Image must belong to a queue!");
    return VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .imageType = toConvert.type,
        .format = toConvert.format,
        .extent = {
            .width = toConvert.width,
            .height = toConvert.height,
            .depth = toConvert.depth,
        },
        .mipLevels = toConvert.mipLevels,
        .arrayLayers = toConvert.arrayLayers,
        .samples = toConvert.samples,
        .tiling = toConvert.tiling,
        .usage = toConvert.usage,
        .sharingMode = toConvert.sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(toConvert.queueFamilyIndices.size()),
        .pQueueFamilyIndices = toConvert.queueFamilyIndices.data(),
        .initialLayout = toConvert.initialLayout,
    };
}

struct VmaImage
{
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    VkImage image = VK_NULL_HANDLE;
};

struct ImageViewState
{
    const VkImage &image;
    const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    const VkFormat &format;
    const VkComponentMapping components = {};
    const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    const uint32_t baseMipLevel = 0;
    const uint32_t levelCount = 1;
    const uint32_t baseArrayLayer = 0;
    const uint32_t layerCount = 1;
};
inline VkImageViewCreateInfo toVkImageViewCreateInfo(const ImageViewState &toConvert)
{
    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .image = toConvert.image,
        .viewType = toConvert.viewType,
        .format = toConvert.format,
        .components = toConvert.components,
        .subresourceRange{
            .aspectMask = toConvert.aspectMask,
            .baseMipLevel = toConvert.baseMipLevel,
            .levelCount = toConvert.levelCount,
            .baseArrayLayer = toConvert.baseArrayLayer,
            .layerCount = toConvert.layerCount,
        },
    };
}

struct FramebufferState
{
    const VkRenderPass &renderPass;
    const std::span<const VkImageView> &attachments;
    const uint32_t &width;
    const uint32_t &height;
    const uint32_t layers = 1;
};
inline VkFramebufferCreateInfo toVkFramebufferCreateInfo(const FramebufferState &toConvert)
{
    return VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .renderPass = toConvert.renderPass,
        .attachmentCount = static_cast<uint32_t>(toConvert.attachments.size()),
        .pAttachments = toConvert.attachments.data(),
        .width = toConvert.width,
        .height = toConvert.height,
        .layers = toConvert.layers,
    };
}