#pragma once
#include "assimp/mesh.h"
#include "assimp/texture.h"
#include "assimp/types.h"
#include "common.h"
#include "stdio.h"
#include "types.h"
#include <string>
#include <vulkan/vulkan_core.h>

Mesh loadMesh(const std::string &filePath, TransferQueue &transferQueue, VmaAllocator &allocator, DeviceInfo &deviceInfo, VkDescriptorPool descriptorPool,
              VkDescriptorSetLayout descriptorSetLayout);
Texture uploadTexture(uint32_t width, uint32_t height, uint32_t numComponents, void *texels, TransferQueue &transferQueue, VmaAllocator allocator,
                      DeviceInfo &deviceInfo, VkFormat imageFormat);
void freeMesh(Mesh &mesh, VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool);
void transferBufferImmediate(const TransferQueue &queue, UploadableBuffer &upload, VkBufferCopy &copyInfo);
VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
void updatePerMeshDescriptor(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorImageInfo &imageInfo, uint32_t binding);
VkSampler createSampler(VkDevice device);
template <typename T>
AllocatedBuffer uploadBuffer(VmaAllocator allocator, std::vector<T> &cpuBuf, const DeviceInfo &deviceInfo, VkBufferUsageFlags usage, TransferQueue &queue)
{
    size_t bufferSize = sizeof(T) * cpuBuf.size();

    VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

    uint32_t queueFamilyIndices[] = {deviceInfo.graphicsQueueFamily, deviceInfo.transferQueueFamily};
    stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;
    stagingBufferInfo.queueFamilyIndexCount = 2;

    VmaAllocationCreateInfo vmaStagingBufAllocInfo = {};
    vmaStagingBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaStagingBufAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuf;
    VmaAllocation stagingAlloc;
    VmaAllocationInfo stagingAllocInfo;
    VK_LOG_ERR(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaStagingBufAllocInfo, &stagingBuf, &stagingAlloc, &stagingAllocInfo));

    memcpy(stagingAllocInfo.pMappedData, cpuBuf.data(), bufferSize);

    VkBufferCreateInfo gpuBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    gpuBufferInfo.size = bufferSize;
    gpuBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;

    VmaAllocationCreateInfo vmagpuBufAllocInfo = {};
    vmagpuBufAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmagpuBufAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkBuffer gpuBuf;
    VmaAllocation gpuAlloc;
    VmaAllocationInfo gpuAllocInfo;
    VK_LOG_ERR(vmaCreateBuffer(allocator, &gpuBufferInfo, &vmagpuBufAllocInfo, &gpuBuf, &gpuAlloc, &gpuAllocInfo));

    UploadableBuffer uploadable = {};
    uploadable.src = stagingBuf;
    uploadable.dst = gpuBuf;

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = bufferSize;

    transferBufferImmediate(queue, uploadable, bufferCopy);
    vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);

    AllocatedBuffer retBuf = {};
    retBuf.buffer = gpuBuf;
    retBuf.allocation = gpuAlloc;
    return retBuf;
}
