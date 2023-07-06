#pragma once
#include "GpuResource.h"
#include <functional>

template <class Derived, typename T>
class BasePerFrame
{
  public:
    virtual Handle<T> curFrameHandle() = 0;
    virtual T *curFrameData() = 0;

    std::array<Handle<T>, 3> m_handles;

  protected:
    BasePerFrame<Derived, T>() = default;
};

class PerFrameBuffer final : public BasePerFrame<PerFrameBuffer, Buffer>, public BaseBuffer<PerFrameBuffer>
{
  public:
    PerFrameBuffer() = default;
    PerFrameBuffer(const Buffer::State &&state);

    virtual Handle<Buffer> curFrameHandle() override;
    virtual Buffer *curFrameData() override;
    virtual void destroy() override;
};

class PerFrameImage final : public BasePerFrame<PerFrameImage, Image>, public BaseImage<PerFrameImage>
{
  public:
    PerFrameImage() = default;
    PerFrameImage(const Image::State &&state);
    PerFrameImage(const SwapchainInfo &swapchainInfo, const WindowInfo &windowInfo);

    void doAll(std::function<void(Image *)> func);
    virtual Handle<Image> curFrameHandle() override;
    virtual Image *curFrameData() override;

    virtual void destroy() override;
    virtual void addImageViewFormat(VkFormat format) override;
    virtual VkImageView getImageViewByFormat(VkFormat format = VK_FORMAT_MAX_ENUM) override;

  private:
    bool m_swapchainImages = false;
};

struct PerFrameImageRef
{
    VkFormat format;
    PerFrameImage image;
};

struct Framebuffer::PerFrameState
{
    const std::span<PerFrameImageRef> &&images;
    const VkRenderPass &renderpass;
};

class PerFrameFramebuffer final : public BasePerFrame<PerFrameFramebuffer, Framebuffer>, public BaseFrameBuffer<PerFrameFramebuffer>
{
  public:
    PerFrameFramebuffer() = default;
    PerFrameFramebuffer(const Framebuffer::PerFrameState &&state);

    virtual Handle<Framebuffer> curFrameHandle() override;
    virtual Framebuffer *curFrameData() override;
    virtual void destroy() override;
};
