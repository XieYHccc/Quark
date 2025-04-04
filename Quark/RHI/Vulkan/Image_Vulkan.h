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

class Image_Vulkan : public Image, public Cookie, public InternalSyncEnabled {
public:
    Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc); // only used for fill swapchain image infomation
    Image_Vulkan(Device_Vulkan* device, const ImageDesc& desc, const ImageInitData* init_data);
    virtual ~Image_Vulkan();
    
    const VkImage GetHandle() const { return m_handle; }
    const VkImageView GetView() const { return m_view; }
    
    bool IsSwapChainImage() const { return m_isSwapChainImage; }
    
private:
    void PrepareCopy(const ImageDesc& desc, const TextureFormatLayout& layout, const ImageInitData* init_data, Ref<Buffer> stage_buffer, std::vector<VkBufferImageCopy>& copys);
    void GenerateMipMap(const ImageDesc& desc, VkCommandBuffer cmd);

    Device_Vulkan* m_device;
    VkImage m_handle;
    VmaAllocation m_allocation;
    VkImageView m_view;
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