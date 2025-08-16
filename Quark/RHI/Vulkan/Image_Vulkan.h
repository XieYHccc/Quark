#pragma once
#include "Quark/RHI/Vulkan/Common_Vulkan.h"
#include "Quark/RHI/Image.h"
#include "Quark/RHI/TextureFormatLayout.h"
#include "Quark/RHI/Vulkan/Cookie.h"
namespace quark::rhi {

constexpr VkImageLayout ConvertImageLayout(ImageLayout layout)
{
    switch (layout) {
    case ImageLayout::GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ImageLayout::TRANSFER_SRC_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ImageLayout::TRANSFER_DST_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ImageLayout::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

constexpr VkImageAspectFlags ConvertImageAspect(uint32_t value)
{
    VkImageAspectFlags ret = 0;
    if (value & IMAGE_ASPECT_COLOR_BIT)
        ret |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (value & IMAGE_ASPECT_DEPTH_BIT)
        ret |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (value & IMAGE_ASPECT_STENCIL_BIT)
        ret |= VK_IMAGE_ASPECT_STENCIL_BIT;
    if (value & IMAGE_ASPECT_LUMINANCE_BIT)
        ret |= VK_IMAGE_ASPECT_PLANE_0_BIT;
    if (value & IMAGE_ASPECT_CHROMINANCE_BIT)
        ret |= VK_IMAGE_ASPECT_PLANE_1_BIT;

    return ret;
}

constexpr VkImageType ConvertImageType(ImageType type)
{
    switch (type) 
    {
    case ImageType::TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case ImageType::TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case ImageType::TYPE_3D:
        return VK_IMAGE_TYPE_3D;
    }
}

class ImageView_Vulkan : public ImageView, public Cookie, public InternalSyncEnabled
{
public:
    ImageView_Vulkan(Device_Vulkan* device, const ImageViewDesc& desc);
    ImageView_Vulkan(Device_Vulkan* device, VkImageView default_view, const ImageViewDesc& desc);
    virtual ~ImageView_Vulkan();

    // By default, gets a combined view which includes all aspects in the image.
    // This would be used mostly for render targets.
    const VkImageView GetView() const { return m_view; }
    const VkImageView GetRTView(uint32_t layer) const;
    const VkImageView GetMipView(uint32_t level) const;

    // Gets an image view which only includes floating point domains.
    // Takes effect when we want to sample from an image which is Depth/Stencil,
    // but we only want to sample depth.
    const VkImageView GetFloatView() const { return m_depth_view != VK_NULL_HANDLE ? m_depth_view : m_view; }

    // Gets an image view which only includes integer domains.
    // Takes effect when we want to sample from an image which is Depth/Stencil,
    // but we only want to sample stencil.
    const VkImageView GetIntegerView() const { return m_stencil_view != VK_NULL_HANDLE ? m_stencil_view : m_view; }


private:
    bool CreateDefalutView(const VkImageViewCreateInfo& info);
    bool CreateRTViews(const ImageDesc& image_desc, const VkImageViewCreateInfo& info);
    bool CreateMipViews(const VkImageViewCreateInfo& info);
    bool CreateAlternateViews(const ImageDesc& image_desc, const VkImageViewCreateInfo& info);

    void FreeView(VkImageView view);

    Device_Vulkan* m_device;
    VkImageView m_view = VK_NULL_HANDLE;
    std::vector<VkImageView> m_render_target_views;
    std::vector<VkImageView> m_mip_views;
    VkImageView m_depth_view = VK_NULL_HANDLE;
    VkImageView m_stencil_view = VK_NULL_HANDLE;

    friend class Device_Vulkan;
};
CONVERT_TO_VULKAN_INTERNAL_FUNC(ImageView)

class Image_Vulkan : public Image, public Cookie, public InternalSyncEnabled 
{
public:
    Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc); // only used for fill swapchain image infomation
    Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data);
    virtual ~Image_Vulkan();
    
    const ImageView& GetDefaultView() const override;
    ImageView& GetDefaultView() override;

    const VkImage GetHandle() const { return m_handle; }
    
    bool IsSwapChainImage() const { return m_isSwapChainImage; }
    
private:
    void PrepareCopy(const ImageDesc& desc, const TextureFormatLayout& layout, const ImageInitData* init_data, Ref<Buffer> stage_buffer, std::vector<VkBufferImageCopy>& copys);
    void GenerateMipMap(const ImageDesc& desc, VkCommandBuffer cmd);

    Device_Vulkan* m_device;
    VkImage m_handle;
    VmaAllocation m_allocation;
    Ref<ImageView_Vulkan> m_default_view;
    bool m_isSwapChainImage;

    friend class Device_Vulkan;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Image)

class Sampler_Vulkan : public Sampler {
public:
    Sampler_Vulkan(Device_Vulkan* device, const SamplerDesc& desc);
    ~Sampler_Vulkan();
    VkSampler GetHandle() const { return m_handle; }
private:
    Device_Vulkan* m_device;
    VkSampler m_handle;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(Sampler)
}