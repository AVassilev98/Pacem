#include "gpuresource.h"
#include "renderer.h"
#include "vkinit.h"
#include <algorithm>
#include <limits>
#include <vulkan/vulkan_core.h>

Buffer::Buffer(const State &state)
    : m_size(state.size)
{
    Renderer &renderer = Renderer::Get();

    std::array<uint32_t, static_cast<uint32_t>(QueueFamily::Size)> queueFamilyIndices;
    for (uint32_t i = 0; i < state.families.size(); i++)
    {
        queueFamilyIndices[i] = renderer.getQueueFamilyIdx(state.families[i]);
    }

    VmaAllocationState allocationState = {
        .allocation = m_allocation,
        .allocationInfo = m_allocationInfo,
        .usage = state.vmaUsage,
        .flags = state.vmaFlags,
    };

    VkInit::BufferState bufferState = {
        .size = state.size,
        .usage = state.usage,
        .queueFamilyIndices = std::span(queueFamilyIndices.data(), state.families.size()),
        .allocation = allocationState,
    };

    m_buffer = VkInit::CreateVkBuffer(bufferState);
}

void Buffer::freeResources()
{
    Renderer &renderer = Renderer::Get();
    vmaDestroyBuffer(renderer.m_vmaAllocator, m_buffer, m_allocation);
}

Image::Image(const State &state)
    : m_width(state.width)
    , m_height(state.height)
    , m_aspectMask(state.aspectMask)
    , m_mutableFormat(state.mutableFormat)
{
    Renderer &renderer = Renderer::Get();

    std::array<uint32_t, static_cast<uint32_t>(QueueFamily::Size)> queueFamilyIndices;
    for (uint32_t i = 0; i < state.families.size(); i++)
    {
        queueFamilyIndices[i] = renderer.getQueueFamilyIdx(state.families[i]);
    }

    VmaAllocationState allocationState = {
        .allocation = m_allocation,
        .allocationInfo = m_allocationInfo,
        .usage = state.vmaUsage,
        .flags = state.vmaFlags,
    };

    VkInit::ImageState imageState = {
        .format = state.format,
        .width = state.width,
        .height = state.height,
        .samples = state.samples,
        .usage = state.usage,
        .queueFamilyIndices = queueFamilyIndices,
        .initialLayout = state.initialLayout,
        .allocation = allocationState,
    };
    m_image = VkInit::CreateVkImage(imageState);
    addImageViewFormat(state.format);
}

void Image::addImageViewFormat(VkFormat format)
{
#if DEBUG
    assert(format != VK_FORMAT_UNDEFINED && "Should not add undefined format view!");
    if (m_numImageViews && !m_mutableFormat)
    {
        assert(m_mutableFormat && "An image view already exists for this immutable image!");
    }

    for (uint32_t i = 0; i < m_numImageViews; i++)
    {
        std::pair<VkFormat, VkImageView> &kv = m_imageViews[i];
        assert(kv.first != format && "Should not add duplicate imageView!");
    }
#endif

    VkInit::ImageViewState imageViewState = {
        .image = m_image,
        .format = format,
        .aspectMask = m_aspectMask,
    };
    m_imageViews[m_numImageViews++] = {format, VkInit::CreateVkImageView(imageViewState)};
}

VkImageView Image::getImageViewByFormat(VkFormat format) const
{
    if (format == VK_FORMAT_MAX_ENUM)
    {
        return m_imageViews[0].second;
    }

    assert(format != VK_FORMAT_UNDEFINED && "Cannot get a view into undefined format");
    for (uint32_t i = 0; i < m_numImageViews; i++)
    {
        const std::pair<VkFormat, VkImageView> &kv = m_imageViews[i];
        if (format == kv.first)
        {
            return kv.second;
        }
    }
    assert(0 && "Could not find imageview format!");
    return VK_NULL_HANDLE;
}

void Image::freeResources()
{
    Renderer &renderer = Renderer::Get();

    for (uint32_t i = 0; i < m_numImageViews; i++)
    {
        vkDestroyImageView(renderer.m_deviceInfo.device, m_imageViews[i].second, nullptr);
    }
    vmaDestroyImage(renderer.m_vmaAllocator, m_image, m_allocation);
}

Framebuffer::Framebuffer(const State &state)
    : m_renderPass(state.renderpass)
{
    m_width = std::numeric_limits<unsigned>::max();
    m_height = std::numeric_limits<unsigned>::max();

    std::vector<VkImageView> attachments;
    attachments.reserve(state.images.size());
    for (const ImageRef &imageRef : state.images)
    {
        attachments.push_back(imageRef.image->getImageViewByFormat(imageRef.format));
        m_width = std::min(m_width, imageRef.image->m_width);
        m_height = std::min(m_height, imageRef.image->m_height);
    }

    VkInit::FramebufferState framebufferState = {
        .renderPass = state.renderpass,
        .attachments = attachments,
        .width = m_width,
        .height = m_height,
    };

    m_frameBuffer = VkInit::CreateVkFramebuffer(framebufferState);
}

void Framebuffer::freeResources()
{
    Renderer &renderer = Renderer::Get();

    vkDestroyFramebuffer(renderer.m_deviceInfo.device, m_frameBuffer, nullptr);
}
