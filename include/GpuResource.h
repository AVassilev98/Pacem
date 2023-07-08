#pragma once
#include <iterator>
#include <span>
#include <vulkan/vulkan_core.h>

#include "Types.h"
#include "vma.h"
#include "vulkan/vulkan.h"

#include "ResourcePool.h"
#include <array>

template <class Derived>
struct BaseBuffer
{
    virtual void destroy() = 0;
};

struct Buffer : public BaseBuffer<Buffer>
{
    struct State;
    virtual void destroy() override;

    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
    uint32_t m_size;

    Buffer() = default;
    Buffer(const State &&state);
    Buffer(const Buffer &buffer) = default;

  private:
};

struct Buffer::State
{
    const uint32_t &size;
    const VkBufferUsageFlags &usage;
    const std::span<QueueFamily> &families;
    VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
};

template <class Derived>
struct BaseImage
{
    virtual void addImageViewFormat(VkFormat format) = 0;
    virtual VkImageView getImageViewByFormat(VkFormat format = VK_FORMAT_MAX_ENUM) = 0;
    virtual void destroy() = 0;
};

struct Image final : public BaseImage<Image>
{
    struct State;
    Image() = default;
    Image(const Image::State &&state);

    Image(const Image &image) = default;
    virtual void addImageViewFormat(VkFormat format) override;
    virtual VkImageView getImageViewByFormat(VkFormat format = VK_FORMAT_MAX_ENUM) override;
    virtual void destroy() override;

    VkImageAspectFlags m_aspectMask;
    VkImage m_image;
    VmaAllocation m_allocation;
    VmaAllocationInfo m_allocationInfo;
    uint32_t m_width;
    uint32_t m_height;
    bool m_mutableFormat;
    bool m_preallocated = false;

    static constexpr uint32_t MaxImageViewTypes = 3;
    uint32_t m_numImageViews = 0;
    std::array<std::pair<VkFormat, VkImageView>, MaxImageViewTypes> m_imageViews = {};
};

struct Image::State
{
    const VkImageUsageFlags &usage;
    const uint32_t &width;
    const uint32_t &height;
    const VkFormat &format;
    const std::span<QueueFamily> &families;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImage preAllocatedImage = VK_NULL_HANDLE;
    bool mutableFormat = false;
};

template <class Derived>
struct BaseFrameBuffer
{
    virtual void destroy() = 0;
};

struct Framebuffer : public BaseFrameBuffer<Framebuffer>
{
    Framebuffer() = default;
    struct State;
    struct PerFrameState;
    Framebuffer(const State &&state);
    void destroy();

    VkFramebuffer m_frameBuffer;
    VkRenderPass m_renderPass;
    uint32_t m_width;
    uint32_t m_height;
};

struct ImageRef
{
    VkFormat format;
    Image *image;
};

struct Framebuffer::State
{
    const std::span<ImageRef> &images;
    const VkRenderPass &renderpass;
};
