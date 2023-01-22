#pragma once
#include "common.h"
#include "stdio.h"
#include "types.h"
#include <string>
#include <vulkan/vulkan_core.h>

Mesh loadMesh(const std::string &filePath, TransferQueue &transferQueue, VmaAllocator &allocator, DeviceInfo &deviceInfo);
void freeMesh(Mesh &mesh, VmaAllocator);
void transferImmediate(const TransferQueue &queue, Uploadable upload, VkBufferCopy copyInfo);
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

    Uploadable uploadable = {};
    uploadable.uploadType = Uploadable::Type::Buffer;
    uploadable.bufferPair = std::pair<VkBuffer, VkBuffer>(stagingBuf, gpuBuf);

    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = 0;
    bufferCopy.dstOffset = 0;
    bufferCopy.size = bufferSize;

    transferImmediate(queue, uploadable, bufferCopy);
    vmaDestroyBuffer(allocator, stagingBuf, stagingAlloc);

    AllocatedBuffer retBuf = {};
    retBuf.buffer = gpuBuf;
    retBuf.allocation = gpuAlloc;
    return retBuf;
}
