#include "Common.h"
#include "GLFW/glfw3.h"
#include "GpuResource.h"
#include "RenderPass.h"
#include "Renderer.h"
#include "Types.h"
#include "VkInit.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "imgui.h"
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <span>
#include <vulkan/vulkan_core.h>

void RenderPass::addMesh(Mesh *mesh)
{
    m_meshes.push_back(mesh);
}

void RenderPass::fulfillRenderPassDependencies(VkCommandBuffer cmd, uint32_t frameIdx)
{
    for (auto &dependency : m_dependencies)
    {
        dependency(cmd, frameIdx);
    }
}

void DeferredRenderPass::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.numFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();
    auto imageFamily = std::to_array<QueueFamily>({QueueFamily::Graphics});

    m_outputImages = Renderer::Get().getSwapchainImages();
    m_diffuseBuffers = PerFrameImage({
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .width = renderer.getSwapchainInfo().width,
        .height = renderer.getSwapchainInfo().height,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .families = imageFamily,
        .mutableFormat = false,
    });
    m_normalBuffers = PerFrameImage({
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .width = renderer.getSwapchainInfo().width,
        .height = renderer.getSwapchainInfo().height,
        .format = VK_FORMAT_R16G16_SFLOAT,
        .families = imageFamily,
        .mutableFormat = false,
    });

    auto attachments = std::to_array<PerFrameImageRef>({
        {VK_FORMAT_B8G8R8A8_UNORM, m_diffuseBuffers},
        {VK_FORMAT_R16G16_SFLOAT, m_normalBuffers},
        {VK_FORMAT_D32_SFLOAT, m_depthImages},
    });

    m_framebuffers = PerFrameFramebuffer({
        .images = std::span(attachments),
        .renderpass = renderPass,
    });

    updateGBuffer();
}

DeferredRenderPass::DeferredRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera)
    : m_cameraRef(camera)
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

    VkInit::RenderPassState::Attachment normals = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_R16G16_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 0,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkInit::RenderPassState::Attachment diffuseColor = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 1,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkInit::RenderPassState::Attachment depth = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 2,
        .referenceLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    auto colorAttachments = std::to_array({diffuseColor, normals});

    VkInit::RenderPassState renderPassState = {
        .colorAttachments = colorAttachments,
        .depthAttachment = &depth,
    };
    VkRenderPass renderPass = VkInit::CreateVkRenderPass(renderPassState);

    auto vertexBindingDescriptions = std::to_array({
        VkVertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    });

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
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto colorBlendStates = std::to_array({
        colorBlendAttachmentState,
        colorBlendAttachmentState,
    });

    GraphicsPipeline::State pipelineState = {
        .shaders = shaders,
        .layouts = descriptorSetLayouts,
        .renderPass = renderPass,
        .pushConstantRanges = std::span(&range, 1),
        .vertexBindingDescription = vertexBindingDescriptions,
        .vertexAttributeDescription = vertexAttributeDescriptions,
        .colorBlendAttachmentStates = colorBlendStates,
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
    };
    m_pipeline = std::move(GraphicsPipeline(pipelineState));
}

void DeferredRenderPass::updateGBuffer()
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.numFramesInFlight();

    m_gBuffer.m_diffuseBuffers = m_diffuseBuffers;
    m_gBuffer.m_normalBuffers = m_normalBuffers;
    m_gBuffer.m_depthImages = m_depthImages;
}

void DeferredRenderPass::resize(uint32_t width, uint32_t height)
{
    m_framebuffers.destroy();
    m_normalBuffers.destroy();
    m_diffuseBuffers.destroy();
    createFrameBuffers(m_pipeline.m_renderPass);
}

void DeferredRenderPass::draw(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    static uint32_t frameCount = 0;
    Renderer &renderer = Renderer::Get();

    glm::mat4 model = glm::rotate(glm::mat4{1.0f}, glm::radians(frameCount * 0.005f), glm::vec3(0, 1, 0))
                    * glm::rotate(glm::mat4{1.0f}, glm::radians(90.0f), glm::vec3(1, 0, 0));
    glm::mat4 view = m_cameraRef.m_view;
    glm::mat4 projection = m_cameraRef.m_projection;
    frameCount++;

    PushConstants pushConstants = {};
    pushConstants.M = model;
    pushConstants.V = view;
    pushConstants.P = projection;

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = renderer.getDrawAreaExtent();
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_pipeline.m_renderPass;

    VkClearValue clearValues[3];
    clearValues[0].color = {0.f, 0.f, 0.f, 0.f};
    clearValues[1].depthStencil = {1.f, 0};
    clearValues[2].color = {0.f, 0.f, 0.f, 0.f};

    renderPassBeginInfo.clearValueCount = 3;
    renderPassBeginInfo.pClearValues = clearValues;

    VkExtent2D windowExtent = renderer.getDrawAreaExtent();
    VkViewport viewport;
    viewport.height = static_cast<float>(windowExtent.height);
    viewport.width = static_cast<float>(windowExtent.width);
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)windowExtent.width, (uint32_t)windowExtent.height};

    renderPassBeginInfo.framebuffer = m_framebuffers.curFrameData()->m_frameBuffer;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.m_pipeline);
    vkCmdPushConstants(commandBuffer, m_pipeline.m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

    for (Mesh *mesh : m_meshes)
    {
        mesh->drawMesh(commandBuffer, m_pipeline.m_pipelineLayout);
    }

    vkCmdEndRenderPass(commandBuffer);
}

DeferredRenderPass::~DeferredRenderPass()
{
    m_diffuseBuffers.destroy();
    m_normalBuffers.destroy();
    m_framebuffers.destroy();
    m_pipeline.freeResources();
}

void EditorRenderPass::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.numFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();

    QueueFamily imageFamily = QueueFamily::Graphics;

    PerFrameImage swapchainImages = Renderer::Get().getSwapchainImages();
    m_depthImages = PerFrameImage(Image::State{
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .width = swapchainInfo.width,
        .height = swapchainInfo.height,
        .format = VK_FORMAT_D32_SFLOAT,
        .families = {&imageFamily, 1},
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
    });
    swapchainImages.addImageViewFormat(VK_FORMAT_B8G8R8A8_UNORM);
    auto attachments = std::to_array<PerFrameImageRef>({
        {VK_FORMAT_B8G8R8A8_UNORM, swapchainImages},
        {VK_FORMAT_D32_SFLOAT, m_depthImages},
    });

    m_framebuffers = PerFrameFramebuffer(Framebuffer::PerFrameState{
        .images = std::span(attachments),
        .renderpass = renderPass,
    });
}

EditorRenderPass::EditorRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera)
    : m_cameraRef(camera)
{
    std::array<VkDescriptorSetLayout, DSL_FREQ_COUNT> descriptorSetLayouts;
    descriptorSetLayouts[DSL_FREQ_PER_FRAME] = VkInit::CreateEmptyVkDescriptorSetLayout();
    descriptorSetLayouts[DSL_FREQ_PER_PASS] = VkInit::CreateEmptyVkDescriptorSetLayout();
    descriptorSetLayouts[DSL_FREQ_PER_MAT] = VkInit::CreateEmptyVkDescriptorSetLayout();
    descriptorSetLayouts[DSL_FREQ_PER_MESH] = VkInit::CreateEmptyVkDescriptorSetLayout();

    VkInit::RenderPassState::Attachment color = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 0,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkInit::RenderPassState::Attachment depth = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 1,
        .referenceLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkInit::RenderPassState renderPassState = {
        .colorAttachments = std::span(&color, 1),
        .depthAttachment = &depth,
    };
    VkRenderPass renderPass = VkInit::CreateVkRenderPass(renderPassState);

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(PushConstants);
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    createFrameBuffers(renderPass);

    GraphicsPipeline::State pipelineState = {
        .enableDepthTest = VK_TRUE,
        .shaders = shaders,
        .layouts = descriptorSetLayouts,
        .renderPass = renderPass,
        .pushConstantRanges = std::span(&range, 1),
        .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        .vertexBindingDescription = std::span<VkVertexInputBindingDescription>(),
        .vertexAttributeDescription = std::span<VkVertexInputAttributeDescription>(),
        .colorBlendAttachmentStates = std::span(&colorBlendAttachmentState, 1),
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
    };
    m_pipeline = std::move(GraphicsPipeline(pipelineState));
}

EditorRenderPass::~EditorRenderPass()
{
    m_framebuffers.destroy();
    m_depthImages.destroy();
    m_pipeline.freeResources();
}

void EditorRenderPass::draw(VkCommandBuffer commandBuffer, uint32_t frameIdx)
{
    static uint32_t frameCount = 0;
    glm::mat4 model;
    glm::mat4 view = m_cameraRef.m_view;
    glm::mat4 projection = m_cameraRef.m_projection;
    frameCount++;

    PushConstants pushConstants = {};
    pushConstants.M = glm::mat4(1);
    pushConstants.V = view;
    pushConstants.P = projection;

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = Renderer::Get().getDrawAreaExtent();
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_pipeline.m_renderPass;

    VkClearValue clearValues[3];
    clearValues[0].color = {0.f, 0.f, 0.f, 1.f};
    clearValues[1].depthStencil = {1.f, 0};

    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    VkExtent2D windowExtent = Renderer::Get().getDrawAreaExtent();
    VkViewport viewport;
    viewport.height = static_cast<float>(windowExtent.height);
    viewport.width = static_cast<float>(windowExtent.width);
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)windowExtent.width, (uint32_t)windowExtent.height};

    renderPassBeginInfo.framebuffer = m_framebuffers.curFrameData()->m_frameBuffer;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
    vkCmdPushConstants(commandBuffer, m_pipeline.m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.m_pipeline);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void EditorRenderPass::resize(uint32_t width, uint32_t height)
{
    m_framebuffers.destroy();
    m_depthImages.destroy();
    createFrameBuffers(m_pipeline.m_renderPass);
}

void DeferredRenderPass::declareImageDependency(PerFrameImage &depthImages)
{
    auto dependencyExecutor = [&](VkCommandBuffer, uint32_t) -> void
    {
        Renderer &renderer = Renderer::Get();
        bool frameBuffersDirty = false;
        if (m_depthImages.curFrameData() == nullptr)
        {
            frameBuffersDirty = true;
        }

        if (frameBuffersDirty)
        {
            // TODO: Nasty hack, we shouldn't have to destroy everything here! RenderGraph should fix it
            m_depthImages = depthImages;
            m_diffuseBuffers.destroy();
            m_normalBuffers.destroy();
            m_framebuffers.destroy();
            createFrameBuffers(m_pipeline.m_renderPass);
        }
    };
    m_dependencies.push_back(dependencyExecutor);
}

void ShadingRenderPass::updateDescriptorSet(VkCommandBuffer buffer, uint32_t frameIdx)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.numFramesInFlight();

    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Previous layout
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;   // New layout
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Image aspect mask
    imageBarrier.subresourceRange.baseMipLevel = 0;                       // Base mip level
    imageBarrier.subresourceRange.levelCount = 1;                         // Number of mip levels
    imageBarrier.subresourceRange.baseArrayLayer = 0;                     // Base array layer
    imageBarrier.subresourceRange.layerCount = 1;                         // Number of layers

    Renderer::DescriptorUpdateState descriptorUpdateState = {
        .descriptorSet = m_lightingDescriptorSets[frameIdx][DSL_FREQ_PER_PASS],
        .imageView = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageSampler = VK_NULL_HANDLE,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    PerFrameImage swapchainImages = renderer.getSwapchainImages();

    imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    imageBarrier.image = m_gBuffer->m_diffuseBuffers.curFrameData()->m_image;
    vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier);
    descriptorUpdateState.imageView = m_gBuffer->m_diffuseBuffers.getImageViewByFormat();
    descriptorUpdateState.binding = 0;
    renderer.updateDescriptor(descriptorUpdateState);

    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageBarrier.image = m_gBuffer->m_normalBuffers.curFrameData()->m_image;
    vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier);
    descriptorUpdateState.imageView = m_gBuffer->m_normalBuffers.getImageViewByFormat();
    descriptorUpdateState.binding = 1;
    renderer.updateDescriptor(descriptorUpdateState);

    imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    imageBarrier.image = swapchainImages.curFrameData()->m_image;
    vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageBarrier);
    descriptorUpdateState.imageView = swapchainImages.curFrameData()->getImageViewByFormat();
    descriptorUpdateState.binding = 2;
    renderer.updateDescriptor(descriptorUpdateState);
}

void ShadingRenderPass::createImages()
{
    Renderer &renderer = Renderer::Get();

    uint32_t maxFramesInFlight = renderer.numFramesInFlight();
    m_lightingDescriptorSets.resize(maxFramesInFlight);
    auto queueFamilies = std::to_array<QueueFamily>({QueueFamily::Graphics});

    PerFrameImage swapchainImages = renderer.getSwapchainImages();
    VkExtent2D windowExtent = renderer.getDrawAreaExtent();

    m_outputImages = PerFrameImage({
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .width = windowExtent.width,
        .height = windowExtent.height,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .families = queueFamilies,
    });
}

void ShadingRenderPass::createDescriptorSets()
{
    Renderer &renderer = Renderer::Get();

    uint32_t maxFramesInFlight = renderer.numFramesInFlight();
    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        m_lightingDescriptorSets[i][DSL_FREQ_PER_PASS] = renderer.allocateDescriptorSet(m_lightingDescriptorSetLayouts[DSL_FREQ_PER_PASS]);
    }
}

ShadingRenderPass::ShadingRenderPass(const Shader &lightCullShader, const Shader &lightingShader, const UserControlledCamera &camera)
    : m_cameraRef(camera)
{
    uint32_t binding;
    VkDescriptorType descriptorType;
    uint32_t descriptorCount;
    VkShaderStageFlags stageFlags;
    const VkSampler *pImmutableSamplers;
    auto lightingDescriptorSetBindings = std::to_array<VkDescriptorSetLayoutBinding>({
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1),
        VkInit::CreateVkDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2),
    });

    m_lightingDescriptorSetLayouts[DSL_FREQ_PER_FRAME] = VkInit::CreateEmptyVkDescriptorSetLayout();
    m_lightingDescriptorSetLayouts[DSL_FREQ_PER_PASS] = VkInit::CreateVkDescriptorSetLayout(lightingDescriptorSetBindings);
    m_lightingDescriptorSetLayouts[DSL_FREQ_PER_MAT] = VkInit::CreateEmptyVkDescriptorSetLayout();
    m_lightingDescriptorSetLayouts[DSL_FREQ_PER_MESH] = VkInit::CreateEmptyVkDescriptorSetLayout();

    VkPushConstantRange range;
    range.offset = 0;
    range.size = sizeof(PushConstants);
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // ComputePipeline::State lightCullPipelineState = {.layout = m_lightCullPipelineLayout, .shader = lightCullShader};
    ComputePipeline::State shadingPipelineState = {.layouts = m_lightingDescriptorSetLayouts, .shader = lightingShader};

    // m_lightCullPipeline = ComputePipeline(lightCullPipelineState);
    m_shadingPipeline = ComputePipeline(shadingPipelineState);

    // createImages();
    createDescriptorSets();
}

ShadingRenderPass::~ShadingRenderPass()
{
    Renderer &renderer = Renderer::Get();

    // m_lightCullPipeline.freeResources();
    m_shadingPipeline.freeResources();

    // vkDestroyPipelineLayout(renderer.m_deviceInfo.device, m_lightCullPipelineLayout, nullptr);
    // vkDestroyPipelineLayout(renderer.m_deviceInfo.device, m_shadingPipelineLayout, nullptr);
}

void ShadingRenderPass::resize(uint32_t width, uint32_t height)
{
}

void ShadingRenderPass::draw(VkCommandBuffer buffer, uint32_t frameIdx)
{
    glm::u32vec3 blockDim = {32, 32, 1};
    assert(m_gBuffer && "GBuffer must be set");

    uint32_t gBufWidth = m_gBuffer->m_diffuseBuffers.curFrameData()->m_width;
    uint32_t gBufHeight = m_gBuffer->m_diffuseBuffers.curFrameData()->m_height;

    PushConstants pushConstants = {
        .M = glm::mat4(),
        .V = m_cameraRef.m_view,
        .P = m_cameraRef.m_projection,
    };

    VkImageSubresourceLayers subresourceLayers = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_shadingPipeline.m_pipeline);
    updateDescriptorSet(buffer, frameIdx);
    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_shadingPipeline.m_pipelineLayout, DSL_FREQ_PER_PASS, 1,
                            &m_lightingDescriptorSets[frameIdx][DSL_FREQ_PER_PASS], 0, nullptr);
    VkClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    // vkCmdClearColorImage(buffer, m_outputImages[frameIdx].m_image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
    vkCmdDispatch(buffer, (gBufWidth + blockDim.x - 1) / blockDim.x, (gBufHeight + blockDim.y - 1) / blockDim.y, 1);

    VkMemoryBarrier renderPassMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };

    VkOffset3D startOffset = {0, 0, 0};
    Image *curImage = m_gBuffer->m_diffuseBuffers.curFrameData();
    VkOffset3D endOffset = {(int32_t)curImage->m_width, (int32_t)curImage->m_height, 1};
    VkImageBlit imageBlit = {
        .srcSubresource = subresourceLayers,
        .srcOffsets = {
            startOffset,
            endOffset,
        },
        .dstSubresource = subresourceLayers,
        .dstOffsets = {
            startOffset,
            endOffset,
        },
    };

    PerFrameImage swapchainImages = Renderer::Get().getSwapchainImages();

    VkImageMemoryBarrier imageBarrierSwapchain = {};
    imageBarrierSwapchain.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrierSwapchain.srcAccessMask = 0;
    imageBarrierSwapchain.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrierSwapchain.oldLayout = VK_IMAGE_LAYOUT_GENERAL;         // Previous layout
    imageBarrierSwapchain.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // New layout
    imageBarrierSwapchain.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrierSwapchain.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrierSwapchain.image = swapchainImages.curFrameData()->m_image;         // The image to transition
    imageBarrierSwapchain.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Image aspect mask
    imageBarrierSwapchain.subresourceRange.baseMipLevel = 0;                       // Base mip level
    imageBarrierSwapchain.subresourceRange.levelCount = 1;                         // Number of mip levels
    imageBarrierSwapchain.subresourceRange.baseArrayLayer = 0;                     // Base array layer
    imageBarrierSwapchain.subresourceRange.layerCount = 1;                         // Number of layers

    auto imageMemoryBarriers = std::to_array<VkImageMemoryBarrier>({imageBarrierSwapchain});

    vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         imageMemoryBarriers.size(), imageMemoryBarriers.data());
}

void ShadingRenderPass::declareGBufferDependency(GBuffer &gBuffer)
{
    auto dependencyExecutor = [&](VkCommandBuffer, uint32_t) -> void
    {
        m_gBuffer = &gBuffer;
    };
    m_dependencies.push_back(dependencyExecutor);
}