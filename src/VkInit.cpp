#include "Common.h"
#include "Renderer.h"
#include "Shader.h"
#include "VkInit.h"
#include "VkTypes.h"
#include <vulkan/vulkan_core.h>

VkPipelineShaderStageCreateInfo VkInit::CreateVkPipelineShaderStageCreateInfo(const Shader &shader)
{
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .stage = shader.m_moduleFlags,
        .module = shader.m_module,
        .pName = "main",
        .pSpecializationInfo = VK_NULL_HANDLE,
    };
}

VkDescriptorSetLayout VkInit::CreateEmptyVkDescriptorSetLayout()
{
    Renderer &renderer = Renderer::Get();
    constexpr VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .bindingCount = 0,
        .pBindings = nullptr,
    };
    VkDescriptorSetLayout descriptorSetLayout;
    VK_LOG_ERR(vkCreateDescriptorSetLayout(renderer.getDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

VkDescriptorSetLayout VkInit::CreateVkDescriptorSetLayout(const std::span<const VkDescriptorSetLayoutBinding> &bindings)
{
    Renderer &renderer = Renderer::Get();
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
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

VkPipelineLayout VkInit::CreateVkPipelineLayout(const PipelineLayoutState &state)
{
    Renderer &renderer = Renderer::Get();
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = toVkPipelineLayoutCreateInfo(state);
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreatePipelineLayout(renderer.getDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

VkPipeline VkInit::CreateVkGraphicsPipeline(const VkGraphicsPipelineCreateInfo &graphicsPipelineCreateInfo)
{
    Renderer &renderer = Renderer::Get();
    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateGraphicsPipelines(renderer.getDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &pipeline));
    return pipeline;
}

VkPipeline VkInit::CreateVkComputePipeline(const VkComputePipelineCreateInfo &state)
{
    Renderer &renderer = Renderer::Get();
    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateComputePipelines(renderer.getDevice(), VK_NULL_HANDLE, 1, &state, nullptr, &pipeline));
    return pipeline;
}

VkRenderPass VkInit::CreateVkRenderPass(const RenderPassState &state)
{
    Renderer &renderer = Renderer::Get();

    auto fillVkAttachmentDescriptions
        = [](std::vector<VkAttachmentDescription> &vkAttachments, const std::span<const AttachmentDescription> &vkinitAttachments)
    {
        for (const AttachmentDescription &attachment : vkinitAttachments)
        {
            vkAttachments.emplace_back(toVkAttachmentDescription(attachment));
        }
    };

    auto createVkAttachmentReferences
        = [](const std::span<const AttachmentDescription> &vkinitAttachments) -> std::vector<VkAttachmentReference>
    {
        std::vector<VkAttachmentReference> out;
        for (const AttachmentDescription &attachment : vkinitAttachments)
        {
            out.emplace_back(toVkAttachmentReference(attachment));
        }
        return out;
    };

    auto createVkSubpassDependencies
        = [](const std::span<const SubpassDependency> &vksubpassDependencies) -> std::vector<VkSubpassDependency>
    {
        std::vector<VkSubpassDependency> out;
        for (const SubpassDependency &subpassDependency : vksubpassDependencies)
        {
            out.emplace_back(toVkSubpassDependency(subpassDependency));
        }
        return out;
    };

    std::vector<VkAttachmentDescription> attachments;
    uint32_t totalAttachments
        = state.colorAttachments.size() + state.inputAttachments.size() + state.depthAttachment.size() + state.resolveAttachments.size();
    attachments.reserve(totalAttachments);

    fillVkAttachmentDescriptions(attachments, state.colorAttachments);
    fillVkAttachmentDescriptions(attachments, state.inputAttachments);
    fillVkAttachmentDescriptions(attachments, state.depthAttachment);
    fillVkAttachmentDescriptions(attachments, state.resolveAttachments);

    std::vector<VkAttachmentReference> colorAttachmentReference = createVkAttachmentReferences(state.colorAttachments);
    std::vector<VkAttachmentReference> inputAttachmentReference = createVkAttachmentReferences(state.inputAttachments);
    std::vector<VkAttachmentReference> depthAttachmentReference = createVkAttachmentReferences(state.depthAttachment);
    std::vector<VkAttachmentReference> resolveAttachmentReference = createVkAttachmentReferences(state.resolveAttachments);

    std::vector<VkSubpassDependency> subpassDependencies = createVkSubpassDependencies(state.dependencies);

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = state.bindpoint;
    subpassDescription.colorAttachmentCount = colorAttachmentReference.size();
    subpassDescription.inputAttachmentCount = inputAttachmentReference.size();
    subpassDescription.pDepthStencilAttachment = depthAttachmentReference.size() ? &depthAttachmentReference[0] : VK_NULL_HANDLE;
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
    VkSamplerCreateInfo samplerCreateInfo = toVkSamplerCreateInfo(state);
    VkSampler sampler = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateSampler(renderer.getDevice(), &samplerCreateInfo, nullptr, &sampler));
    return sampler;
}

VmaBuffer VkInit::CreateVkBuffer(const BufferState &state)
{
    Renderer &renderer = Renderer::Get();
    VkBufferCreateInfo bufferCreateInfo = toVkBufferCreateInfo(state);
    VmaBuffer buffer;
    VK_LOG_ERR(vmaCreateBuffer(renderer.getAllocator(), &bufferCreateInfo, &state.allocation, &buffer.buffer, &buffer.allocation,
                               &buffer.allocationInfo));
    return buffer;
}

VmaImage VkInit::CreateVkImage(const ImageState &state)
{
    Renderer &renderer = Renderer::Get();
    VkImageCreateInfo imageCreateInfo = toVkImageCreateInfo(state);
    VmaImage image;
    VK_LOG_ERR(vmaCreateImage(renderer.getAllocator(), &imageCreateInfo, &state.allocation, &image.image, &image.allocation,
                              &image.allocationInfo));
    return image;
}

VkImageView VkInit::CreateVkImageView(const ImageViewState &state)
{
    Renderer &renderer = Renderer::Get();
    VkImageViewCreateInfo imageViewCreateInfo = toVkImageViewCreateInfo(state);
    VkImageView imageView = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateImageView(renderer.getDevice(), &imageViewCreateInfo, nullptr, &imageView));
    return imageView;
}

VkFramebuffer VkInit::CreateVkFramebuffer(const FramebufferState &state)
{
    Renderer &renderer = Renderer::Get();
    VkFramebufferCreateInfo framebufferCreateInfo = toVkFramebufferCreateInfo(state);
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateFramebuffer(renderer.getDevice(), &framebufferCreateInfo, nullptr, &framebuffer));
    return framebuffer;
}