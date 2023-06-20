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
        bool mutableFormat = false;
    };
    Image(const State &state);

    Image(const Image &image) = default;
    void addImageViewFormat(VkFormat format);
    VkImageView getImageViewByFormat(VkFormat format = VK_FORMAT_MAX_ENUM) const;
    void freeResources();

    VkImageAspectFlags m_aspectMask;
    VkImage m_image;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
    uint32_t m_width;
    uint32_t m_height;
    bool m_mutableFormat;

    static constexpr uint32_t MaxImageViewTypes = 3;
    uint32_t m_numImageViews = 0;
    std::array<std::pair<VkFormat, VkImageView>, MaxImageViewTypes> m_imageViews = {};
};

struct ImageRef
{
    VkFormat format;
    Image *image;
};

struct Framebuffer
{
    Framebuffer() = default;
    struct State
    {
        const std::span<ImageRef> &images;
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