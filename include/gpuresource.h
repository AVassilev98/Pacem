#pragma once
#include <span>
#include <vulkan/vulkan_core.h>

#include "types.h"
#include "vma.h"
#include "vulkan/vulkan.h"

struct Buffer
{
    Buffer() = default;
    struct State
    {
        const uint32_t &size;
        const VkBufferUsageFlags &usage;
        const std::span<QueueFamily> &families;
        VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    };
    Buffer(const State &state);
    Buffer(const Buffer &buffer) = default;
    void freeResources();

    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
    uint32_t m_size;
};

struct Image
{
    Image() = default;
    struct State
    {
        const uint32_t &width;
        const uint32_t &height;
        const VkImageUsageFlags &usage;
        const VkFormat &format;
        const std::span<QueueFamily> &families;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    };
    Image(const State &state);

    struct CopyState
    {
        const VkImage &image;
        const VkImageView &imageView;
        const VmaAllocation &allocation;
        const uint32_t &width;
        const uint32_t &height;
    };
    Image(const CopyState &state);
    Image(const Image &image) = default;
    void freeResources();

    VkImage m_image;
    VkImageView m_imageView;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
    uint32_t m_width;
    uint32_t m_height;
};

struct Framebuffer
{
    Framebuffer() = default;
    struct State
    {
        const std::span<Image *> &images;
        const VkRenderPass &renderpass;
    };
    Framebuffer(const State &state);
    Framebuffer(const Framebuffer &framebuffer) = default;
    void freeResources();

    VkFramebuffer m_frameBuffer;
    VkRenderPass m_renderPass;
    uint32_t m_width;
    uint32_t m_height;
};