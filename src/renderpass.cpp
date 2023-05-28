#include "renderer.h"
#include "renderpass.h"
#include "types.h"
#include "vkinit.h"
#include <array>
#include <span>
#include <vulkan/vulkan_core.h>

void MainRenderPass::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.getMaxNumFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();

    m_multisampledImages.reserve(maxFramesInFlight);
    m_depthImages.reserve(maxFramesInFlight);
    m_swapchainImages.reserve(maxFramesInFlight);
    m_framebuffers.reserve(maxFramesInFlight);

    QueueFamily imageFamily = QueueFamily::Graphics;
    Image::State msaaState = {
        .width = swapchainInfo.width,
        .height = swapchainInfo.height,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .format = renderer.m_windowInfo.preferredSurfaceFormat.format,
        .families = {&imageFamily, 1},
        .samples = renderer.m_numSamples,
    };

    Image::State depthBufState = {
        .width = swapchainInfo.width,
        .height = swapchainInfo.height,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .format = VK_FORMAT_D32_SFLOAT,
        .families = {&imageFamily, 1},
        .samples = renderer.m_numSamples,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    };

    for (int i = 0; i < maxFramesInFlight; i++)
    {
        m_multisampledImages.emplace_back(msaaState);
        m_depthImages.emplace_back(depthBufState);
        m_swapchainImages.emplace_back(&renderer.m_swapchainImages[i]);
        auto attachments = std::to_array({&m_multisampledImages[i], &m_depthImages[i], m_swapchainImages[i]});

        Framebuffer::State framebufferState = {
            .images = std::span(attachments),
            .renderpass = renderPass,
        };
        m_framebuffers.emplace_back(framebufferState);
    }
}

MainRenderPass::MainRenderPass(const std::span<Shader *> &shaders)
{
    Renderer &renderer = Renderer::Get();

    auto perMaterialDescriptorSetDesc = std::to_array({
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
    });

    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> descriptorSetLayouts;
    descriptorSetLayouts[DSL_FREQ_PER_FRAME] = VkInit::CreateEmptyVkDescriptorSetLayout();
    descriptorSetLayouts[DSL_FREQ_PER_PASS] = VkInit::CreateEmptyVkDescriptorSetLayout();
    descriptorSetLayouts[DSL_FREQ_PER_MAT] = VkInit::CreateVkDescriptorSetLayout(perMaterialDescriptorSetDesc);
    descriptorSetLayouts[DSL_FREQ_PER_MESH] = VkInit::CreateEmptyVkDescriptorSetLayout();

    VkInit::RenderPassState::Attachment color = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .format = Renderer::Get().m_windowInfo.preferredSurfaceFormat.format,
        .samples = Renderer::Get().m_numSamples,
        .attachment = 0,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkInit::RenderPassState::Attachment depth = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = Renderer::Get().m_numSamples,
        .attachment = 1,
        .referenceLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkInit::RenderPassState::Attachment resolve = {
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .format = Renderer::Get().m_windowInfo.preferredSurfaceFormat.format,
        .attachment = 2,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    auto subpassDependencies = std::to_array({
        VkInit::RenderPassState::SubpassDependency{
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        },
        VkInit::RenderPassState::SubpassDependency{
            .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        },
    });

    VkInit::RenderPassState renderPassState = {
        .colorAttachments = std::span(&color, 1),
        .depthAttachment = &depth,
        .resolveAttachments = std::span(&resolve, 1),
        .dependencies = subpassDependencies,
    };
    VkRenderPass renderPass = VkInit::CreateVkRenderPass(renderPassState);

    auto vertexBindingDescriptions = std::to_array({
        VkVertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    });

    VkVertexInputAttributeDescription vertexAttributes[4] = {};
    vertexAttributes[0].binding = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[0].offset = offsetof(Vertex, position);

    vertexAttributes[1].binding = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[1].offset = offsetof(Vertex, normal);

    vertexAttributes[2].binding = 0;
    vertexAttributes[2].location = 2;
    vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributes[2].offset = offsetof(Vertex, color);

    vertexAttributes[3].binding = 0;
    vertexAttributes[3].location = 3;
    vertexAttributes[3].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributes[3].offset = offsetof(Vertex, textureCoordinate);

    auto vertexAttributeDescriptions = std::to_array({
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, position),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, color),
        },
        VkVertexInputAttributeDescription{
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, textureCoordinate),
        },
    });

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(PushConstants);
    range.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Create attachments
    createFrameBuffers(renderPass);

    Pipeline::State pipelineState = {
        .shaders = shaders,
        .layouts = descriptorSetLayouts,
        .renderPass = renderPass,
        .pushConstantRanges = std::span(&range, 1),
        .vertexBindingDescription = vertexBindingDescriptions,
        .vertexAttributeDescription = vertexAttributeDescriptions,
        .sampleCount = renderer.m_numSamples,
    };
    m_pipeline = Pipeline(pipelineState);
}

void MainRenderPass::resize(uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
    {
        m_framebuffers[i].freeResources();
        m_multisampledImages[i].freeResources();
        m_depthImages[i].freeResources();
    }
    m_framebuffers.clear();
    m_multisampledImages.clear();
    m_depthImages.clear();
    m_swapchainImages.clear();
    createFrameBuffers(m_pipeline.m_renderPass);
}
