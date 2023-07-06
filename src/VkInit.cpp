#include "Common.h"
#include "Renderer.h"
#include "Shader.h"
#include "VkInit.h"
#include <vulkan/vulkan_core.h>

VkPipelineShaderStageCreateInfo VkInit::CreateVkPipelineShaderStageCreateInfo(const Shader &shader)
{
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipelineShaderStageCreateInfo.stage = shader.m_moduleFlags;
    pipelineShaderStageCreateInfo.module = shader.m_module;
    pipelineShaderStageCreateInfo.pName = "main";

    return pipelineShaderStageCreateInfo;
}

VkDescriptorSetLayout VkInit::CreateEmptyVkDescriptorSetLayout()
{
    Renderer &renderer = Renderer::Get();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptorSetLayoutInfo.bindingCount = 0;
    descriptorSetLayoutInfo.pBindings = nullptr;

    VkDescriptorSetLayout descriptorSetLayout;
    VK_LOG_ERR(vkCreateDescriptorSetLayout(renderer.getDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

VkDescriptorSetLayout VkInit::CreateVkDescriptorSetLayout(const std::span<VkDescriptorSetLayoutBinding> &bindings)
{
    Renderer &renderer = Renderer::Get();

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptorSetLayoutInfo.bindingCount = bindings.size();
    descriptorSetLayoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    VK_LOG_ERR(vkCreateDescriptorSetLayout(renderer.getDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

VkDescriptorSetLayoutBinding VkInit::CreateVkDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stages, uint32_t binding)
{
    return VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
        .stageFlags = stages,
    };
}

VkPipelineLayout VkInit::CreateVkPipelineLayout(const std::span<VkDescriptorSetLayout, DSL_FREQ_COUNT> &dsls,
                                                std::span<VkPushConstantRange> pushRanges)
{
    Renderer &renderer = Renderer::Get();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.setLayoutCount = dsls.size();
    pipelineLayoutCreateInfo.pSetLayouts = dsls.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushRanges.size();
    pipelineLayoutCreateInfo.pPushConstantRanges = pushRanges.data();

    VkPipelineLayout pipelineLayout;
    VK_LOG_ERR(vkCreatePipelineLayout(renderer.getDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

VkPipeline VkInit::CreateVkGraphicsPipeline(const GraphicsPipelineState &state)
{
    Renderer &renderer = Renderer::Get();

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(state.vertexInputBindingDescription.size()),
        .pVertexBindingDescriptions = state.vertexInputBindingDescription.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(state.vertexInputAttributeDescription.size()),
        .pVertexAttributeDescriptions = state.vertexInputAttributeDescription.data(),
    };

    VkPipelineViewportStateCreateInfo viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = static_cast<uint32_t>(state.viewports.size()),
        .pViewports = state.viewports.data(),
        .scissorCount = static_cast<uint32_t>(state.scissors.size()),
        .pScissors = state.scissors.data(),
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(state.dynamicStates.size()),
        .pDynamicStates = state.dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stageCount = state.shaderInfo.size();
    pipelineCreateInfo.pStages = state.shaderInfo.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputStateInfo;
    pipelineCreateInfo.pInputAssemblyState = &state.inputAssemblyStateInfo;
    pipelineCreateInfo.pTessellationState = &state.tesselationStateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &state.rasterStateInfo;
    pipelineCreateInfo.pMultisampleState = &state.msaaStateInfo;
    pipelineCreateInfo.pDepthStencilState = &state.depthStencilInfo;
    pipelineCreateInfo.pColorBlendState = &state.colorBlendStateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
    pipelineCreateInfo.layout = state.layout;
    pipelineCreateInfo.renderPass = state.renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineHandle = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateGraphicsPipelines(renderer.getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));
    return pipeline;
}

VkPipeline VkInit::CreateVkComputePipeline(const ComputePipelineState &state)
{
    Renderer &renderer = Renderer::Get();

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .flags = 0,
        .stage = state.shader,
        .layout = state.layout,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateComputePipelines(renderer.getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline));
    return pipeline;
}

VkRenderPass VkInit::CreateVkRenderPass(const RenderPassState &state)
{
    Renderer &renderer = Renderer::Get();

    auto fillVkAttachmentDescriptions
        = [](std::vector<VkAttachmentDescription> &vkAttachments, const std::span<RenderPassState::Attachment> &vkinitAttachments)
    {
        for (const RenderPassState::Attachment &attachment : vkinitAttachments)
        {
            vkAttachments.push_back(VkAttachmentDescription{
                .format = attachment.format,
                .samples = attachment.samples,
                .loadOp = attachment.loadOp,
                .storeOp = attachment.storeOp,
                .stencilLoadOp = attachment.stencilLoadOp,
                .stencilStoreOp = attachment.stencilStoreOp,
                .initialLayout = attachment.initialLayout,
                .finalLayout = attachment.finalLayout,
            });
        }
    };

    auto createVkAttachmentReferences
        = [](const std::span<RenderPassState::Attachment> &vkinitAttachments) -> std::vector<VkAttachmentReference>
    {
        std::vector<VkAttachmentReference> out;
        for (const RenderPassState::Attachment &attachment : vkinitAttachments)
        {
            out.push_back(VkAttachmentReference{
                .attachment = attachment.attachment,
                .layout = attachment.referenceLayout,
            });
        }
        return out;
    };

    auto createVkSubpassDependencies
        = [](const std::span<RenderPassState::SubpassDependency> &vksubpassDependencies) -> std::vector<VkSubpassDependency>
    {
        std::vector<VkSubpassDependency> out;
        for (const RenderPassState::SubpassDependency &subpassDependency : vksubpassDependencies)
        {
            out.push_back(VkSubpassDependency{
                .srcSubpass = subpassDependency.srcSubpass,
                .dstSubpass = subpassDependency.dstSubpass,
                .srcStageMask = subpassDependency.srcStageMask,
                .dstStageMask = subpassDependency.dstStageMask,
                .srcAccessMask = subpassDependency.srcAccessMask,
                .dstAccessMask = subpassDependency.dstAccessMask,
            });
        }
        return out;
    };

    std::vector<VkAttachmentDescription> attachments;
    uint32_t totalAttachments
        = state.colorAttachments.size() + state.inputAttachments.size() + (state.depthAttachment ? 1 : 0) + state.resolveAttachments.size();
    attachments.reserve(totalAttachments);

    fillVkAttachmentDescriptions(attachments, state.colorAttachments);
    fillVkAttachmentDescriptions(attachments, state.inputAttachments);
    if (state.depthAttachment)
    {
        fillVkAttachmentDescriptions(attachments, std::span(state.depthAttachment, 1));
    }
    fillVkAttachmentDescriptions(attachments, state.resolveAttachments);

    std::vector<VkAttachmentReference> colorAttachmentReference = createVkAttachmentReferences(state.colorAttachments);
    std::vector<VkAttachmentReference> inputAttachmentReference = createVkAttachmentReferences(state.inputAttachments);
    std::vector<VkAttachmentReference> depthAttachmentReference = createVkAttachmentReferences(
        state.depthAttachment ? std::span(state.depthAttachment, 1) : std::span<RenderPassState::Attachment, 0>());
    std::vector<VkAttachmentReference> resolveAttachmentReference = createVkAttachmentReferences(state.resolveAttachments);

    std::vector<VkSubpassDependency> subpassDependencies = createVkSubpassDependencies(state.dependencies);

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = state.bindpoint;
    subpassDescription.colorAttachmentCount = colorAttachmentReference.size();
    subpassDescription.inputAttachmentCount = inputAttachmentReference.size();
    subpassDescription.pDepthStencilAttachment = state.depthAttachment ? depthAttachmentReference.data() : VK_NULL_HANDLE;
    subpassDescription.pColorAttachments = colorAttachmentReference.data();
    subpassDescription.pInputAttachments = inputAttachmentReference.data();
    subpassDescription.pResolveAttachments = resolveAttachmentReference.data();

    VkSubpassDependency subpassDependency = {};

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.dependencyCount = subpassDependencies.size();
    renderPassInfo.pDependencies = subpassDependencies.data();

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateRenderPass(renderer.getDevice(), &renderPassInfo, nullptr, &renderPass));
    return renderPass;
}

VkSampler VkInit::CreateVkSampler(const SamplerState &state)
{
    Renderer &renderer = Renderer::Get();

    VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = state.magFilter,
        .minFilter = state.minFilter,
        .mipmapMode = state.mipmapMode,
        .addressModeU = state.addressModeU,
        .addressModeV = state.addressModeV,
        .addressModeW = state.addressModeW,
        .mipLodBias = 0.0f,
        .anisotropyEnable = state.anisotropyEnable,
        .maxAnisotropy = state.maxAnisotropy,
        .compareEnable = state.compareEnable,
        .compareOp = state.compareOp,
        .minLod = state.minLod,
        .maxLod = state.maxLod,
        .borderColor = state.borderColor,
        .unnormalizedCoordinates = state.unnormalizedCoordinates,
    };

    VkSampler sampler = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateSampler(renderer.getDevice(), &samplerCreateInfo, nullptr, &sampler));
    return sampler;
}

VkBuffer VkInit::CreateVkBuffer(const BufferState &state)
{
    Renderer &renderer = Renderer::Get();

    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = state.size,
        .usage = state.usage,
        .sharingMode = state.sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(state.queueFamilyIndices.size()),
        .pQueueFamilyIndices = state.queueFamilyIndices.data(),
    };

    VmaAllocationCreateInfo vmaAllocationInfo = {
        .flags = state.allocation.flags,
        .usage = state.allocation.usage,
    };

    VkBuffer buffer = VK_NULL_HANDLE;
    VK_LOG_ERR(vmaCreateBuffer(renderer.getAllocator(), &bufferCreateInfo, &vmaAllocationInfo, &buffer, &state.allocation.allocation,
                               &state.allocation.allocationInfo));
    return buffer;
}

VkImage VkInit::CreateVkImage(const ImageState &state)
{
    Renderer &renderer = Renderer::Get();
    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = state.type,
        .format = state.format,
        .extent = {.width = state.width, .height = state.height, .depth = state.depth,},
        .mipLevels = state.mipLevels,
        .arrayLayers = state.arrayLayers,
        .samples = state.samples,
        .tiling = state.tiling,
        .usage = state.usage,
        .sharingMode = state.sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(state.queueFamilyIndices.size()),
        .pQueueFamilyIndices = state.queueFamilyIndices.data(),
        .initialLayout = state.initialLayout,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = state.allocation.flags,
        .usage = state.allocation.usage,
    };

    VkImage image = VK_NULL_HANDLE;
    VK_LOG_ERR(vmaCreateImage(renderer.getAllocator(), &imageCreateInfo, &allocationCreateInfo, &image, &state.allocation.allocation,
                              &state.allocation.allocationInfo));
    return image;
}

VkImageView VkInit::CreateVkImageView(const ImageViewState &state)
{
    Renderer &renderer = Renderer::Get();

    VkImageSubresourceRange subresourceRange = {
        .aspectMask = state.aspectMask,
        .baseMipLevel = state.baseMipLevel,
        .levelCount = state.levelCount,
        .baseArrayLayer = state.baseArrayLayer,
        .layerCount = state.layerCount,
    };

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = state.image,
        .viewType = state.viewType,
        .format = state.format,
        .components = state.components,
        .subresourceRange = subresourceRange,
    };

    VkImageView imageView = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateImageView(renderer.getDevice(), &imageViewCreateInfo, nullptr, &imageView));
    return imageView;
}

VkFramebuffer VkInit::CreateVkFramebuffer(const FramebufferState &state)
{
    Renderer &renderer = Renderer::Get();

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = state.renderPass,
        .attachmentCount = static_cast<uint32_t>(state.attachments.size()),
        .pAttachments = state.attachments.data(),
        .width = state.width,
        .height = state.height,
        .layers = state.layers,
    };

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateFramebuffer(renderer.getDevice(), &framebufferCreateInfo, nullptr, &framebuffer));
    return framebuffer;
}