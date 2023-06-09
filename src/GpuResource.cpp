#include "GpuResource.h"
#include "Renderer.h"
#include "Types.h"
#include "VkInit.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vulkan/vulkan_core.h>

Buffer::Buffer(const State &&state)
    : m_size(state.size)
{
    Renderer &renderer = Renderer::Get();

    std::array<uint32_t, static_cast<uint32_t>(QueueFamily::Size)> queueFamilyIndices;
    for (uint32_t i = 0; i < state.families.size(); i++)
    {
        queueFamilyIndices[i] = renderer.getQueueFamilyIdx(state.families[i]);
    }
    VmaBuffer output = VkInit::CreateVkBuffer({
        .size = state.size,
        .usage = state.usage,
        .queueFamilyIndices = queueFamilyIndices,
        .allocation{
            .flags = state.vmaFlags,
            .usage = state.vmaUsage,
        },
    });

    m_buffer = output.buffer;
    m_allocation = output.allocation;
    m_allocationInfo = output.allocationInfo;
}

void Buffer::destroy()
{
    Renderer &renderer = Renderer::Get();
    vmaDestroyBuffer(renderer.getAllocator(), m_buffer, m_allocation);
}

Image::Image(const State &&state)
    : m_width(state.width)
    , m_height(state.height)
    , m_aspectMask(state.aspectMask)
    , m_mutableFormat(state.mutableFormat)
    , m_preallocated(state.preAllocatedImage)
{
    Renderer &renderer = Renderer::Get();

    if (state.preAllocatedImage == VK_NULL_HANDLE)
    {
        std::array<uint32_t, static_cast<uint32_t>(QueueFamily::Size)> queueFamilyIndices;
        for (uint32_t i = 0; i < state.families.size(); i++)
        {
            queueFamilyIndices[i] = renderer.getQueueFamilyIdx(state.families[i]);
        }
        VmaImage image = VkInit::CreateVkImage({
            .format = state.format,
            .width = state.width,
            .height = state.height,
            .samples = state.samples,
            .usage = state.usage,
            .queueFamilyIndices = queueFamilyIndices,
            .initialLayout = state.initialLayout,
            .allocation{
                .flags = state.vmaFlags,
                .usage = state.vmaUsage,
            },
        });

        m_image = image.image;
        m_allocation = image.allocation;
        m_allocationInfo = image.allocationInfo;
    }
    else
    {
        m_image = state.preAllocatedImage;
    }
    addImageViewFormat(state.format);
}

void Image::addImageViewFormat(VkFormat format)
{
#ifndef NDEBUG
    assert(format != VK_FORMAT_UNDEFINED && "Should not add undefined format view!");
    if (m_numImageViews && !m_mutableFormat)
    {
        assert(m_mutableFormat && "An image view already exists for this immutable image!");
    }

    for (uint32_t i = 0; i < m_numImageViews; i++)
    {
        std::pair<VkFormat, VkImageView> &kv = m_imageViews[i];
        assert(kv.first == format && "Should not add duplicate imageView!");
    }
#endif
    m_imageViews[m_numImageViews++] = {
        format,
        VkInit::CreateVkImageView({
            .image = m_image,
            .format = format,
            .aspectMask = m_aspectMask,
        }),
    };
}

VkImageView Image::getImageViewByFormat(VkFormat format)
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

void Image::destroy()
{
    Renderer &renderer = Renderer::Get();

    for (uint32_t i = 0; i < m_numImageViews; i++)
    {
        vkDestroyImageView(renderer.getDevice(), m_imageViews[i].second, nullptr);
    }
    if (!m_preallocated)
    {
        vmaDestroyImage(renderer.getAllocator(), m_image, m_allocation);
    }
}

Framebuffer::Framebuffer(const State &&state)
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

    m_frameBuffer = VkInit::CreateVkFramebuffer({
        .renderPass = state.renderpass,
        .attachments = attachments,
        .width = m_width,
        .height = m_height,
    });
}

void Framebuffer::destroy()
{
    Renderer &renderer = Renderer::Get();

    vkDestroyFramebuffer(renderer.getDevice(), m_frameBuffer, nullptr);
}