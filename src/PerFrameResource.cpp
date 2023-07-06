#include "PerFrameResource.h"

#include "GpuResource.h"
#include "Renderer.h"
#include "ResourcePool.h"
#include "Types.h"
#include "VkInit.h"
#include <array>

namespace
{
    template <typename T, class... Args>
    void createForEachFrameInFlight(std::span<Handle<T>> data, Args &&...args)
    {
        Renderer &renderer = Renderer::Get();
        for (uint32_t i = 0; i < renderer.numFramesInFlight(); i++)
        {
            data[i] = renderer.create(std::forward<Args>(args)...);
        }
    }

    template <typename T, typename Func, class... Args>
    void doForEachFrameInFlight(std::span<Handle<T>> data, Func func, Args &&...args)
    {
        Renderer &renderer = Renderer::Get();
        for (uint32_t i = 0; i < renderer.numFramesInFlight(); i++)
        {
            T *resource = renderer.get(data[i]);
            assert(resource && "Trying to do operation on null resource!");
            (resource->*func)(std::forward<Args>(args)...);
        }
    }

    template <typename T>
    Handle<T> curFrameHandle(std::span<Handle<T>> handles)
    {
        return handles[Renderer::Get().curFrame()];
    }

    template <typename T>
    T *curFrameData(std::span<Handle<T>> handles)
    {
        Renderer &renderer = Renderer::Get();
        return renderer.get(handles[renderer.curFrame()]);
    }

    template <typename T>
    void destroy(std::span<Handle<T>> handles)
    {
        Renderer &renderer = Renderer::Get();
        for (uint32_t i = 0; i < renderer.numFramesInFlight(); i++)
        {
            renderer.destroy(handles[i]);
        }
    }

} // namespace

PerFrameBuffer::PerFrameBuffer(const Buffer::State &&state)
{
    createForEachFrameInFlight<Buffer>(m_handles, std::move(state));
}

Handle<Buffer> PerFrameBuffer::curFrameHandle()
{
    return ::curFrameHandle(std::span<Handle<Buffer>>(m_handles));
}

Buffer *PerFrameBuffer::curFrameData()
{
    return ::curFrameData(std::span<Handle<Buffer>>(m_handles));
}

void PerFrameBuffer::destroy()
{
    ::destroy(std::span<Handle<Buffer>>(m_handles));
}

PerFrameImage::PerFrameImage(const Image::State &&state)
{
    createForEachFrameInFlight<Image>(m_handles, std::move(state));
}

void PerFrameImage::addImageViewFormat(VkFormat format)
{
    doForEachFrameInFlight<Image>(m_handles, &Image::addImageViewFormat, format);
}

VkImageView PerFrameImage::getImageViewByFormat(VkFormat format)
{
    Image *curImage = curFrameData();
    assert(curImage && "Trying to get imageView of null image!");
    return curImage->getImageViewByFormat(format);
}

PerFrameImage::PerFrameImage(const SwapchainInfo &swapchainInfo, const WindowInfo &windowInfo)
{
    m_swapchainImages = true;
    Renderer &renderer = Renderer::Get();

    std::vector<Image> images(renderer.numFramesInFlight());
    std::vector<VkImage> vkImages(renderer.numFramesInFlight());

    uint32_t swapchainImageCount = 0;
    VK_LOG_ERR(vkGetSwapchainImagesKHR(renderer.getDevice(), swapchainInfo.swapchain, &swapchainImageCount, nullptr));
    VK_LOG_ERR(vkGetSwapchainImagesKHR(renderer.getDevice(), swapchainInfo.swapchain, &swapchainImageCount, vkImages.data()));

    auto queueFamilies = std::to_array<QueueFamily>({QueueFamily::Graphics});
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        m_handles[i] = renderer.create({
            .usage = 0, // not allocated with VMA
            .width = swapchainInfo.width,
            .height = swapchainInfo.height,
            .format = windowInfo.preferredSurfaceFormat.format,
            .families = queueFamilies,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .vmaFlags = 0, // not allocated with VMA
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .preAllocatedImage = vkImages[i],
            .mutableFormat = true,
        });
    }
}

Handle<Image> PerFrameImage::curFrameHandle()
{
    return ::curFrameHandle(std::span<Handle<Image>>(m_handles));
}

Image *PerFrameImage::curFrameData()
{
    return ::curFrameData(std::span<Handle<Image>>(m_handles));
}

void PerFrameImage::destroy()
{
    ::destroy(std::span<Handle<Image>>(m_handles));
}

PerFrameFramebuffer::PerFrameFramebuffer(const Framebuffer::PerFrameState &&state)
{
    Renderer &renderer = Renderer::Get();

    std::vector<ImageRef> imageRefs;
    imageRefs.reserve(state.images.size());
    for (uint32_t i = 0; i < renderer.numFramesInFlight(); i++)
    {
        for (PerFrameImageRef &imageRef : state.images)
        {
            imageRefs.push_back(ImageRef{
                .format = imageRef.format,
                .image = renderer.get(imageRef.image.m_handles[i]),
            });
        }
        m_handles[i] = renderer.create(Framebuffer::State{
            .images = imageRefs,
            .renderpass = state.renderpass,
        });
        imageRefs.clear();
    }
}

Handle<Framebuffer> PerFrameFramebuffer::curFrameHandle()
{
    return ::curFrameHandle(std::span<Handle<Framebuffer>>(m_handles));
}

Framebuffer *PerFrameFramebuffer::curFrameData()
{
    return ::curFrameData(std::span<Handle<Framebuffer>>(m_handles));
}

void PerFrameFramebuffer::destroy()
{
    ::destroy(std::span<Handle<Framebuffer>>(m_handles));
}
