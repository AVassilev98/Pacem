#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "gpuresource.h"
#include "imgui.h"
#include "renderer.h"
#include "renderpass.h"
#include "types.h"
#include "vkinit.h"
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

void MainRenderPass::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.getMaxNumFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();

    m_outputImages.resize(maxFramesInFlight);
    m_framebuffers.resize(maxFramesInFlight);

    QueueFamily imageFamily = QueueFamily::Graphics;
    for (int i = 0; i < maxFramesInFlight; i++)
    {
        m_outputImages[i] = &renderer.m_swapchainImages[i];
        auto attachments = std::to_array<ImageRef>({
            {VK_FORMAT_B8G8R8A8_SRGB, m_multisampledImages[i]},
            {VK_FORMAT_D32_SFLOAT, m_depthImages[i]},
            {VK_FORMAT_B8G8R8A8_SRGB, m_outputImages[i]},
        });

        Framebuffer::State framebufferState = {
            .images = std::span(attachments),
            .renderpass = renderPass,
        };
        m_framebuffers[i] = Framebuffer(framebufferState);
    }
}

MainRenderPass::MainRenderPass(const std::span<Shader *> &shaders, const UserControlledCamera &camera)
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

    VkInit::RenderPassState::Attachment color = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .format = Renderer::Get().m_windowInfo.preferredSurfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 0,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkInit::RenderPassState::Attachment depth = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 1,
        .referenceLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkInit::RenderPassState::Attachment resolve = {
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .attachment = 2,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // auto subpassDependencies = std::to_array({
    //     VkInit::RenderPassState::SubpassDependency{
    //         .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //         .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //         .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //     },
    //     VkInit::RenderPassState::SubpassDependency{
    //         .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    //         .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    //         .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    //     },
    // });

    VkInit::RenderPassState renderPassState = {
        .colorAttachments = std::span(&color, 1),
        .depthAttachment = &depth,
        .resolveAttachments = std::span(&resolve, 1),
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
    range.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    GraphicsPipeline::State pipelineState = {
        .shaders = shaders,
        .layouts = descriptorSetLayouts,
        .renderPass = renderPass,
        .pushConstantRanges = std::span(&range, 1),
        .vertexBindingDescription = vertexBindingDescriptions,
        .vertexAttributeDescription = vertexAttributeDescriptions,
        .colorBlendAttachmentState = colorBlendAttachmentState,
        .sampleCount = renderer.m_numSamples,
    };
    m_pipeline = std::move(GraphicsPipeline(pipelineState));
}

void MainRenderPass::resize(uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < m_framebuffers.size(); i++)
    {
        m_framebuffers[i].freeResources();
    }
    createFrameBuffers(m_pipeline.m_renderPass);
}

void MainRenderPass::draw(VkCommandBuffer commandBuffer, uint32_t frameIndex)
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
    renderArea.extent = {m_outputImages[0]->m_width, m_outputImages[0]->m_height};
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_pipeline.m_renderPass;

    VkClearValue clearValues[3];
    clearValues[0].color = {0.f, 0.f, 0.f, 0.f};
    clearValues[1].depthStencil = {1.f, 0};
    clearValues[2].color = {0.f, 0.f, 0.f, 0.f};

    renderPassBeginInfo.clearValueCount = 3;
    renderPassBeginInfo.pClearValues = clearValues;

    VkExtent2D windowExtent = renderer.getWindowExtent();
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

    renderPassBeginInfo.framebuffer = m_framebuffers[frameIndex].m_frameBuffer;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.m_pipeline);
    vkCmdPushConstants(commandBuffer, m_pipeline.m_pipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(PushConstants), &pushConstants);

    for (Mesh *mesh : m_meshes)
    {
        mesh->drawMesh(commandBuffer, m_pipeline.m_pipelineLayout);
    }

    vkCmdEndRenderPass(commandBuffer);
}

MainRenderPass::~MainRenderPass()
{
    for (uint32_t i = 0; i < m_outputImages.size(); i++)
    {
        m_framebuffers[i].freeResources();
    }
    m_pipeline.freeResources();
}

void EditorRenderPass::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.getMaxNumFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();

    m_multisampledImages.reserve(maxFramesInFlight);
    m_depthImages.reserve(maxFramesInFlight);

    QueueFamily imageFamily = QueueFamily::Graphics;
    Image::State msaaState = {
        .width = swapchainInfo.width,
        .height = swapchainInfo.height,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .format = VK_FORMAT_B8G8R8A8_SRGB,
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

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        m_multisampledImages.push_back(msaaState);
        m_depthImages.push_back(depthBufState);
        auto attachments = std::to_array<ImageRef>({
            {VK_FORMAT_B8G8R8A8_SRGB, &m_multisampledImages[i]},
            {VK_FORMAT_D32_SFLOAT, &m_depthImages[i]},
        });

        Framebuffer::State framebufferState = {
            .images = std::span(attachments),
            .renderpass = renderPass,
        };
        m_framebuffers.emplace_back(framebufferState);
    }
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
        .format = Renderer::Get().m_windowInfo.preferredSurfaceFormat.format,
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
        .colorBlendAttachmentState = colorBlendAttachmentState,
        .sampleCount = VK_SAMPLE_COUNT_1_BIT,
    };
    m_pipeline = std::move(GraphicsPipeline(pipelineState));
}

EditorRenderPass::~EditorRenderPass()
{
    for (uint32_t i = 0; i < m_multisampledImages.size(); i++)
    {
        m_framebuffers[i].freeResources();
        m_multisampledImages[i].freeResources();
        m_depthImages[i].freeResources();
    }
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
    pushConstants.M = model;
    pushConstants.V = view;
    pushConstants.P = projection;

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = {m_multisampledImages[0].m_width, m_multisampledImages[0].m_height};
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_pipeline.m_renderPass;

    VkClearValue clearValues[3];
    clearValues[0].color = {0.f, 0.f, 0.f, 0.f};
    clearValues[1].depthStencil = {1.f, 0};

    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    VkExtent2D windowExtent = Renderer::Get().getWindowExtent();
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

    renderPassBeginInfo.framebuffer = m_framebuffers[frameIdx].m_frameBuffer;

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
    for (uint32_t i = 0; i < m_multisampledImages.size(); i++)
    {
        m_framebuffers[i].freeResources();
        m_multisampledImages[i].freeResources();
        m_depthImages[i].freeResources();
    }
    m_framebuffers.clear();
    m_multisampledImages.clear();
    m_depthImages.clear();
    createFrameBuffers(m_pipeline.m_renderPass);
}

void MainRenderPass::declareImageDependency(std::vector<Image> &colorImages, std::vector<Image> &depthImages)
{
    auto dependencyExecutor = [&](VkCommandBuffer, uint32_t) -> void
    {
        Renderer &renderer = Renderer::Get();
        bool frameBuffersDirty = false;

        m_multisampledImages.resize(renderer.getMaxNumFramesInFlight());
        m_depthImages.resize(renderer.getMaxNumFramesInFlight());

        for (uint32_t i = 0; i < Renderer::Get().getMaxNumFramesInFlight(); i++)
        {
            if (m_multisampledImages[i] != &colorImages[i])
            {
                frameBuffersDirty = true;
                m_multisampledImages[i] = &colorImages[i];
            }
            if (m_depthImages[i] != &depthImages[i])
            {
                frameBuffersDirty = true;
                m_depthImages[i] = &depthImages[i];
            }
        }

        if (frameBuffersDirty)
        {
            for (uint32_t i = 0; i < m_framebuffers.size(); i++)
            {
                m_framebuffers[i].freeResources();
            }
            createFrameBuffers(m_pipeline.m_renderPass);
        }
    };
    m_dependencies.push_back(dependencyExecutor);
}
