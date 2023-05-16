#include "glm/fwd.hpp"
#include "meshLoader.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "array"
#include "common.h"
#include "iostream"
#include "vector"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vma.h"

#include "common.h"
#include "renderer.h"
#include "types.h"

#include "shader.h"
#include "vkinit.h"

VkPipeline createPipeline(const VkDevice device, const VkRenderPass renderPass, const VkPipelineLayout pipelineLayout,
                          const std::vector<VkPipelineShaderStageCreateInfo> &shaders)
{
    VkVertexInputBindingDescription vertexInputBindingDescription = {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(Vertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 4;
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo tesselationCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = nullptr;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterCreateInfo.lineWidth = 1.0f;
    rasterCreateInfo.depthBiasEnable = VK_TRUE;
    rasterCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterCreateInfo.depthBiasClamp = 0.0f;
    rasterCreateInfo.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo MSAACreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MSAACreateInfo.rasterizationSamples = Renderer::Get().m_numSamples;
    MSAACreateInfo.minSampleShading = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_TRUE;
    depthStencilCreateInfo.minDepthBounds = 0.1f;
    depthStencilCreateInfo.maxDepthBounds = 1.0f;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateCreateInfo.dynamicStateCount = ARR_CNT(dynamicStates);
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stageCount = shaders.size();
    pipelineCreateInfo.pStages = shaders.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pTessellationState = &tesselationCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterCreateInfo;
    pipelineCreateInfo.pMultisampleState = &MSAACreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineHandle = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_LOG_ERR(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &pipeline));
    return pipeline;
}

void transferBufferImmediate(const TransferQueue &queue, UploadableBuffer &upload, VkBufferCopy &copyInfo)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(queue.immCmdBuf, &commandBufferBeginInfo);
    vkCmdCopyBuffer(queue.immCmdBuf, upload.src, upload.dst, 1, &copyInfo);
    vkEndCommandBuffer(queue.immCmdBuf);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &queue.immCmdBuf;

    vkQueueSubmit(queue.transferQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(queue.transferQueue);
}

static VkCommandPool createCommandPool(const VkDevice device, uint32_t queueFamilyIndex)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool commandPool = nullptr;
    VK_LOG_ERR(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
    return commandPool;
}

static VkCommandBuffer createCommandBuffer(const DeviceInfo &deviceInfo, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer commandBuffer = nullptr;
    VK_LOG_ERR(vkAllocateCommandBuffers(deviceInfo.device, &commandBufferAllocateInfo, &commandBuffer));
    return commandBuffer;
}

static RenderContext createRenderContext(const DeviceInfo &deviceInfo, const SwapchainInfo &swapchainInfo)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    RenderContext renderContext = {};
    renderContext.commandPools.resize(swapchainInfo.numImages);
    renderContext.commandBuffers.resize(swapchainInfo.numImages);
    renderContext.imgAvailableSem.resize(swapchainInfo.numImages);
    renderContext.renderDoneSem.resize(swapchainInfo.numImages);
    renderContext.fences.resize(swapchainInfo.numImages);

    for (uint32_t i = 0; i < swapchainInfo.numImages; i++)
    {
        renderContext.commandPools[i] = createCommandPool(deviceInfo.device, deviceInfo.graphicsQueueFamily);
        renderContext.commandBuffers[i] = createCommandBuffer(deviceInfo, renderContext.commandPools[i]);
        VK_LOG_ERR(vkCreateSemaphore(deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.imgAvailableSem[i]));
        VK_LOG_ERR(vkCreateSemaphore(deviceInfo.device, &semaphoreCreateInfo, nullptr, &renderContext.renderDoneSem[i]));
        VK_LOG_ERR(vkCreateFence(deviceInfo.device, &fenceCreateInfo, nullptr, &renderContext.fences[i]));
    }

    return renderContext;
}

void transferTextureImmediate(const DeviceInfo &deviceInfo, const TransferQueue &queue, UploadableTexture &upload, VkBufferImageCopy &copyInfo,
                              const VkImageSubresourceRange &subresourceRange)
{
    VkCommandBuffer transferCmdBuffer = createCommandBuffer(deviceInfo, queue.transferCommandPool);
    VkCommandBuffer graphicsCmdBuffer = createCommandBuffer(deviceInfo, queue.graphicsCommandPool);

    VkImageMemoryBarrier imageToTransfer = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageToTransfer.srcAccessMask = 0;
    imageToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageToTransfer.srcQueueFamilyIndex = deviceInfo.transferQueueFamily;
    imageToTransfer.dstQueueFamilyIndex = deviceInfo.transferQueueFamily;
    imageToTransfer.image = upload.dst;
    imageToTransfer.subresourceRange = subresourceRange;

    VkImageMemoryBarrier imageOwnershipTransferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageOwnershipTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageOwnershipTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageOwnershipTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageOwnershipTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageOwnershipTransferBarrier.srcQueueFamilyIndex = deviceInfo.transferQueueFamily;
    imageOwnershipTransferBarrier.dstQueueFamilyIndex = deviceInfo.graphicsQueueFamily;
    imageOwnershipTransferBarrier.image = upload.dst;
    imageOwnershipTransferBarrier.subresourceRange = subresourceRange;

    VkImageMemoryBarrier imageToReadable = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageToReadable.srcQueueFamilyIndex = deviceInfo.graphicsQueueFamily;
    imageToReadable.dstQueueFamilyIndex = deviceInfo.graphicsQueueFamily;
    imageToReadable.image = upload.dst;
    imageToReadable.subresourceRange = subresourceRange;

    VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore transferSem;
    vkCreateSemaphore(deviceInfo.device, &semCreateInfo, nullptr, &transferSem);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(transferCmdBuffer, &commandBufferBeginInfo);
    vkCmdPipelineBarrier(transferCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageToTransfer);
    vkCmdCopyBufferToImage(transferCmdBuffer, upload.src, upload.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

    vkCmdPipelineBarrier(transferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageOwnershipTransferBarrier);
    vkEndCommandBuffer(transferCmdBuffer);

    VkSubmitInfo transferSubmission = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    transferSubmission.commandBufferCount = 1;
    transferSubmission.pCommandBuffers = &transferCmdBuffer;
    transferSubmission.signalSemaphoreCount = 1;
    transferSubmission.pSignalSemaphores = &transferSem;

    vkQueueSubmit(queue.transferQueue, 1, &transferSubmission, nullptr);
    vkBeginCommandBuffer(graphicsCmdBuffer, &commandBufferBeginInfo);
    vkCmdPipelineBarrier(graphicsCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageToReadable);
    vkEndCommandBuffer(graphicsCmdBuffer);

    VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    VkSubmitInfo graphicsSubmission = {};
    graphicsSubmission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmission.commandBufferCount = 1;
    graphicsSubmission.pCommandBuffers = &graphicsCmdBuffer;
    graphicsSubmission.pWaitDstStageMask = &waitStageFlags;
    graphicsSubmission.pWaitSemaphores = &transferSem;
    graphicsSubmission.waitSemaphoreCount = 1;

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence transferFence;

    vkCreateFence(deviceInfo.device, &fence_create_info, nullptr, &transferFence);
    vkQueueSubmit(queue.graphicsQueue, 1, &graphicsSubmission, transferFence);
    vkWaitForFences(deviceInfo.device, 1, &transferFence, VK_TRUE, UINT64_MAX);

    vkFreeCommandBuffers(deviceInfo.device, queue.transferCommandPool, 1, &transferCmdBuffer);
    vkFreeCommandBuffers(deviceInfo.device, queue.graphicsCommandPool, 1, &graphicsCmdBuffer);
    vkDestroySemaphore(deviceInfo.device, transferSem, nullptr);
    vkDestroyFence(deviceInfo.device, transferFence, nullptr);
}

Texture uploadTexture(uint32_t width, uint32_t height, uint32_t numComponents, void *texels, TransferQueue &transferQueue, VmaAllocator allocator,
                      DeviceInfo &deviceInfo, VkFormat imageFormat)
{
    uint32_t queueFamilyIndices[] = {deviceInfo.graphicsQueueFamily, deviceInfo.transferQueueFamily};

    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = imageFormat;
    imageCreateInfo.extent = {width, height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 1;
    imageCreateInfo.pQueueFamilyIndices = &deviceInfo.transferQueueFamily;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo vmaDstImgAllocInfo = {};
    vmaDstImgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaDstImgAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkImage textureImage;
    VmaAllocation textureAllocation;
    VmaAllocationInfo textureAllocationInfo;
    VK_LOG_ERR(vmaCreateImage(allocator, &imageCreateInfo, &vmaDstImgAllocInfo, &textureImage, &textureAllocation, &textureAllocationInfo));

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.layerCount = 1;
    subresourceRange.levelCount = 1;

    VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewCreateInfo.image = textureImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageFormat;
    imageViewCreateInfo.components = {};
    imageViewCreateInfo.subresourceRange = subresourceRange;

    VkImageView textureImageView;
    VK_LOG_ERR(vkCreateImageView(deviceInfo.device, &imageViewCreateInfo, nullptr, &textureImageView));

    VmaAllocationCreateInfo vmaStagingBufferAllocInfo = {};
    vmaStagingBufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaStagingBufferAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBufferCreateInfo stagingBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufferCreateInfo.size = width * height * numComponents;
    stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBufferCreateInfo.queueFamilyIndexCount = 1;
    stagingBufferCreateInfo.pQueueFamilyIndices = &deviceInfo.transferQueueFamily;

    VkBuffer stagingBuffer;
    VmaAllocation stagingBufferAllocation;
    VmaAllocationInfo stagingBufferAllocationInfo;

    VK_LOG_ERR(vmaCreateBuffer(allocator, &stagingBufferCreateInfo, &vmaStagingBufferAllocInfo, &stagingBuffer, &stagingBufferAllocation,
                               &stagingBufferAllocationInfo));
    memcpy(stagingBufferAllocationInfo.pMappedData, texels, width * height * numComponents);

    UploadableTexture uploadableTexture = {};
    uploadableTexture.src = stagingBuffer;
    uploadableTexture.dst = textureImage;

    VkImageSubresourceLayers subResourceLayers = {};
    subResourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subResourceLayers.mipLevel = 0;
    subResourceLayers.baseArrayLayer = 0;
    subResourceLayers.layerCount = 1;

    VkBufferImageCopy bufferImageCopy;
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource = subResourceLayers;
    bufferImageCopy.imageOffset = {0, 0, 0};
    bufferImageCopy.imageExtent = {width, height, 1};

    transferTextureImmediate(deviceInfo, transferQueue, uploadableTexture, bufferImageCopy, subresourceRange);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

    Texture texture = {};
    texture.image = textureImage;
    texture.allocation = textureAllocation;
    texture.imageView = textureImageView;

    return texture;
}

VkDescriptorPool createDescriptorPool(VkDevice device)
{
    static const std::vector<VkDescriptorPoolSize> sizes = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolCreateInfo.maxSets = 100;
    descriptorPoolCreateInfo.poolSizeCount = sizes.size();
    descriptorPoolCreateInfo.pPoolSizes = sizes.data();
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool descriptorPool;
    VK_LOG_ERR(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
}

VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet;
    VK_LOG_ERR(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));
    return descriptorSet;
}

void updatePerMeshDescriptor(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorImageInfo &imageInfo, uint32_t binding)
{
    VkWriteDescriptorSet writeDiffuseDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDiffuseDescriptorSet.dstSet = descriptorSet;
    writeDiffuseDescriptorSet.dstBinding = binding;
    writeDiffuseDescriptorSet.dstArrayElement = 0;
    writeDiffuseDescriptorSet.descriptorCount = 1;
    writeDiffuseDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDiffuseDescriptorSet.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &writeDiffuseDescriptorSet, 0, nullptr);
}

VkSampler createSampler(VkDevice device)
{
    VkSamplerCreateInfo samplerCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerCreateInfo.borderColor = {VK_BORDER_COLOR_INT_TRANSPARENT_BLACK};
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VkSampler textureSampler;
    VK_LOG_ERR(vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler));
    return textureSampler;
}

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

    Mesh suzanneMesh = loadMesh(suzannePath, renderer.m_transferQueue, renderer.m_vmaAllocator, renderer.m_deviceInfo, renderer.m_descriptorPool,
                                renderer.m_descriptorSetLayout);

    std::vector<VkPipelineShaderStageCreateInfo> shaders = {};
    shaders.push_back(VkInit::PipelineShaderStageCreateInfo(vertShader));
    shaders.push_back(VkInit::PipelineShaderStageCreateInfo(geoShader));
    shaders.push_back(VkInit::PipelineShaderStageCreateInfo(fragShader));

    VkPipeline pipeline = createPipeline(renderer.m_deviceInfo.device, renderer.m_renderPass, renderer.m_pipelineLayout, shaders);

    double lastTime = glfwGetTime();

    int numMonitors = 1;
    int numModes = 1;
    int refreshRate = 60;
    GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);
    for (int i = 0; i < numMonitors; ++i)
    {
        const GLFWvidmode *modes = glfwGetVideoModes(monitors[i], &numModes);
        for (int j = 0; j < numModes; ++j)
        {
            refreshRate = std::max(refreshRate, modes[j].refreshRate);
        }
    }

    std::cout << refreshRate << "Hz" << std::endl;
    double delay = 1.0 / refreshRate;
    std::cout << "Target delay = " << delay << std::endl;

    int nbFrames = 0;

    while (!renderer.exitSignal())
    {
        while (glfwGetTime() - lastTime < delay)
        {
        }
        VkResult drawStatus = renderer.draw(suzanneMesh, pipeline);
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

    freeMesh(suzanneMesh, renderer.m_deviceInfo.device, renderer.m_vmaAllocator, renderer.m_descriptorPool);
    vkDestroyPipeline(renderer.m_deviceInfo.device, pipeline, nullptr);
    return 0;
}