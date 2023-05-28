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

    VkInit::ImageViewState imageViewState = {
        .image = m_image,
        .format = state.format,
        .aspectMask = state.aspectMask,
    };
    m_imageView = VkInit::CreateVkImageView(imageViewState);
}

Image::Image(const CopyState &state)
    : m_image(state.image)
    , m_imageView(state.imageView)
    , m_allocation(state.allocation)
    , m_width(state.width)
    , m_height(state.height)
{
}

void Image::freeResources()
{
    Renderer &renderer = Renderer::Get();

    vkDestroyImageView(renderer.m_deviceInfo.device, m_imageView, nullptr);
    vmaDestroyImage(renderer.m_vmaAllocator, m_image, m_allocation);
}

Framebuffer::Framebuffer(const State &state)
    : m_renderPass(state.renderpass)
{
    m_width = std::numeric_limits<unsigned>::max();
    m_height = std::numeric_limits<unsigned>::max();

    std::vector<VkImageView> attachments;
    attachments.reserve(state.images.size());
    for (const Image *image : state.images)
    {
        attachments.push_back(image->m_imageView);
        m_width = std::min(m_width, image->m_width);
        m_height = std::min(m_height, image->m_height);
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
