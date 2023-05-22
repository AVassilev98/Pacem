#include "array"
#include "common.h"
#include "glm/fwd.hpp"
#include "iostream"
#include "vector"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vma.h"

#include "common.h"
#include "pipeline.h"
#include "renderer.h"
#include "shader.h"
#include "types.h"
#include "vkinit.h"

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Could not init glfw" << std::endl;
        return -1;
    }

    Renderer &renderer = Renderer::Get();

    std::string vertShaderPath = CONCAT(SHADER_PATH, "default.vert.spv");
    std::string geoShaderPath = CONCAT(SHADER_PATH, "default.geom.spv");
    std::string fragShaderPath = CONCAT(SHADER_PATH, "default.frag.spv");

    std::string suzannePath = CONCAT(ASSET_PATH, "DamagedHelmet.glb");

    Shader vertShader(vertShaderPath, Shader::Stage::Vertex);
    Shader geoShader(geoShaderPath, Shader::Stage::Geometry);
    Shader fragShader(fragShaderPath, Shader::Stage::Fragment);

    auto shadersp = std::to_array({&vertShader, &geoShader, &fragShader});

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

    Pipeline::State pipelineState = {
        .shaders = shadersp,
        .layouts = descriptorSetLayouts,
        .renderPass = renderPass,
        .pushConstantRanges = std::span(&range, 1),
        .vertexBindingDescription = vertexBindingDescriptions,
        .vertexAttributeDescription = vertexAttributeDescriptions,
        .sampleCount = renderer.m_numSamples,
    };
    Pipeline pipeline(pipelineState);
    renderer.m_pipelineLayout = pipeline.m_pipelineLayout;
    renderer.m_renderPass = pipeline.m_renderPass;

    Mesh suzanneMesh(suzannePath, pipeline);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!renderer.exitSignal())
    {
        VkResult drawStatus = renderer.draw(suzanneMesh, pipeline.m_pipeline);
        if (drawStatus == VK_ERROR_OUT_OF_DATE_KHR || drawStatus == VK_SUBOPTIMAL_KHR)
        {
            renderer.resize();
        }
        else
        {
            VK_LOG_ERR(drawStatus);
        }

        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0)
        {
            printf("%f ms/frame\n", 1000.0 / double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }
        glfwPollEvents();
    }
    renderer.wait();

    return 0;
}